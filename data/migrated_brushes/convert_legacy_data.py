#!/usr/bin/env python3
from __future__ import annotations

import argparse
import copy
import io
import json
import re
import shutil
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable
import xml.etree.ElementTree as ET


PALETTE_ORDER = ["Terrain", "Doodad", "Collection", "Item", "Creature", "Raw"]
PALETTE_TOKEN_MAP = {
    "terrain": "Terrain",
    "collections": "Collection",
    "doodad": "Doodad",
    "items": "Item",
    "creatures": "Creature",
    "raw": "Raw",
}
MODULE_SECTIONS = (
    ("borders", "borders/", False),
    ("brushes", "brushes/", False),
    ("creatures", "creatures/", False),
    ("items", "items/", False),
    ("tilesets", "tilesets/", True),
)
ITEM_REGISTRY_FILES = ("items.xml", "items2.xml")
XML_DECLARATION = '<?xml version="1.0" encoding="utf-8"?>\n'
BARE_AMPERSAND_RE = re.compile(r"&(?!#\d+;|#x[0-9A-Fa-f]+;|[A-Za-z_][A-Za-z0-9._-]*;)")
WINDOWS_RESERVED_NAMES = {
    "CON",
    "PRN",
    "AUX",
    "NUL",
    "COM1",
    "COM2",
    "COM3",
    "COM4",
    "COM5",
    "COM6",
    "COM7",
    "COM8",
    "COM9",
    "LPT1",
    "LPT2",
    "LPT3",
    "LPT4",
    "LPT5",
    "LPT6",
    "LPT7",
    "LPT8",
    "LPT9",
}


@dataclass(slots=True)
class FileIssue:
    path: str
    messages: list[str] = field(default_factory=list)


@dataclass(slots=True)
class TopLevelNode:
    source_file: str
    root_tag: str
    tag: str
    sequence: int
    element: ET.Element


@dataclass(slots=True)
class LegacyVersionData:
    version: str
    version_dir: Path
    include_files: list[str]
    metaitems: list[int]
    borders: list[TopLevelNode]
    brushes: list[TopLevelNode]
    tilesets: list[TopLevelNode]
    items: list[TopLevelNode]
    creatures: list[TopLevelNode]
    items_otb: Path | None
    issues: list[FileIssue] = field(default_factory=list)


@dataclass(slots=True)
class ModularTileset:
    name: str
    wrapper: str
    file_path: str
    source_file: str
    sequence: int
    children: list[ET.Element]

    @property
    def palettes(self) -> list[str]:
        return wrapper_to_palettes(self.wrapper)


@dataclass(slots=True)
class NormalizedVersionData:
    version: str
    metaitems: list[int]
    borders: list[ET.Element]
    brushes: list[ET.Element]
    items: list[ET.Element]
    creatures: list[ET.Element]
    tilesets: list[ModularTileset]
    palette_includes: dict[str, list[str]]
    issues: list[FileIssue]
    validation: dict[str, object]
    items_otb: Path | None


def clone_element(element: ET.Element) -> ET.Element:
    return copy.deepcopy(element)


def strip_invalid_xml_chars(text: str) -> str:
    cleaned: list[str] = []
    for char in text:
        code = ord(char)
        if code in (0x9, 0xA, 0xD) or 0x20 <= code <= 0xD7FF or 0xE000 <= code <= 0xFFFD or 0x10000 <= code <= 0x10FFFF:
            cleaned.append(char)
    return "".join(cleaned)


def sanitize_xml_text(path: Path) -> tuple[str, list[str]]:
    raw = path.read_bytes()
    messages: list[str] = []

    if b"\x00" in raw:
        raw = raw.replace(b"\x00", b"")
        messages.append("removed NUL bytes")

    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("cp1252", errors="replace")
        messages.append("decoded with cp1252 fallback")

    stripped = strip_invalid_xml_chars(text)
    if stripped != text:
        text = stripped
        messages.append("removed invalid XML control characters")

    escaped_ampersands = BARE_AMPERSAND_RE.sub("&amp;", text)
    if escaped_ampersands != text:
        text = escaped_ampersands
        messages.append("escaped malformed ampersands")

    if text.startswith("\ufeff"):
        text = text.lstrip("\ufeff")
        messages.append("removed UTF-8 BOM")

    return text, messages


def parse_top_level_elements(path: Path) -> tuple[str, list[ET.Element], list[str]]:
    text, messages = sanitize_xml_text(path)
    root_tag: str | None = None
    root_element: ET.Element | None = None
    top_level: list[ET.Element] = []
    stack: list[ET.Element] = []

    try:
        for event, elem in ET.iterparse(io.StringIO(text), events=("start", "end")):
            if event == "start":
                stack.append(elem)
                if root_tag is None:
                    root_tag = elem.tag
                    root_element = elem
                continue

            if len(stack) == 2:
                top_level.append(clone_element(elem))
                if root_element is not None:
                    try:
                        root_element.remove(elem)
                    except ValueError:
                        pass
                elem.clear()

            stack.pop()
    except ET.ParseError as exc:
        raise RuntimeError(f"{path}: parse failed after sanitization: {exc}") from exc

    if root_tag is None:
        raise RuntimeError(f"{path}: XML document is empty")

    return root_tag, top_level, messages


def parse_root(path: Path) -> tuple[ET.Element, list[str]]:
    text, messages = sanitize_xml_text(path)
    try:
        root = ET.fromstring(text)
    except ET.ParseError as exc:
        raise RuntimeError(f"{path}: parse failed after sanitization: {exc}") from exc
    return root, messages


def expand_item_ids(element: ET.Element) -> list[int]:
    if "id" in element.attrib:
        return [int(element.attrib["id"])]
    if "fromid" not in element.attrib:
        return []
    start = int(element.attrib["fromid"])
    end = int(element.attrib.get("toid", element.attrib["fromid"]))
    if end < start:
        start, end = end, start
    return list(range(start, end + 1))


def item_coverage(elements: Iterable[ET.Element]) -> set[int]:
    covered: set[int] = set()
    for element in elements:
        covered.update(expand_item_ids(element))
    return covered


def wrapper_to_palettes(wrapper: str) -> list[str]:
    if wrapper in PALETTE_TOKEN_MAP:
        return [PALETTE_TOKEN_MAP[wrapper]]

    palettes: list[str] = []
    for token in wrapper.split("_and_"):
        palette = PALETTE_TOKEN_MAP.get(token)
        if palette and palette not in palettes:
            palettes.append(palette)
    return palettes


def encode_filename_component(text: str) -> str:
    text = re.sub(r"\s+", " ", text.replace("/", " ").replace("\\", " ")).strip()
    encoded: list[str] = []
    for index, char in enumerate(text):
        code = ord(char)
        safe = char.isalnum() or char in (" ", "-", "_", "(", ")", ".", ",", "&", "'")
        if safe and char not in '<>:"/\\|?*':
            if index == 0 and char == " ":
                encoded.append(f"~{code:04x}")
            elif index == len(text) - 1 and char in (" ", "."):
                encoded.append(f"~{code:04x}")
            else:
                encoded.append(char)
        else:
            encoded.append(f"~{code:04x}")

    result = "".join(encoded).strip()
    if not result:
        result = "unnamed"
    if result.upper() in WINDOWS_RESERVED_NAMES:
        result = f"{result}~"
    return result


def compact_item_elements(item_ids: Iterable[int]) -> list[ET.Element]:
    sorted_ids = sorted(set(item_ids))
    if not sorted_ids:
        return []

    result: list[ET.Element] = []
    start = previous = sorted_ids[0]
    for item_id in sorted_ids[1:]:
        if item_id == previous + 1:
            previous = item_id
            continue
        result.append(range_item_element(start, previous))
        start = previous = item_id
    result.append(range_item_element(start, previous))
    return result


def range_item_element(start: int, end: int) -> ET.Element:
    if start == end:
        return ET.Element("item", {"id": str(start)})
    return ET.Element("item", {"fromid": str(start), "toid": str(end)})


def indent_tree(element: ET.Element) -> None:
    ET.indent(element, space="    ")


def write_xml(path: Path, root: ET.Element) -> None:
    indent_tree(root)
    xml_text = ET.tostring(root, encoding="unicode")
    path.write_text(XML_DECLARATION + xml_text + "\n", encoding="utf-8")


class LegacyVersionReader:
    def __init__(self, base_dir: Path) -> None:
        self.base_dir = base_dir

    def discover_versions(self) -> list[Path]:
        return sorted(
            (path for path in self.base_dir.iterdir() if path.is_dir() and path.name.isdigit()),
            key=lambda path: int(path.name),
        )

    def read(self, version_dir: Path) -> LegacyVersionData:
        issues: list[FileIssue] = []
        materials_path = version_dir / "materials.xml"
        materials_root, messages = parse_root(materials_path)
        if messages:
            issues.append(FileIssue(path=materials_path.name, messages=messages))
        if materials_root.tag != "materials":
            raise RuntimeError(f"{materials_path}: expected <materials> root, got <{materials_root.tag}>")

        include_files: list[str] = []
        metaitems: list[int] = []
        for child in materials_root:
            if child.tag == "include" and "file" in child.attrib:
                include_files.append(child.attrib["file"])
            elif child.tag == "metaitem" and "id" in child.attrib:
                metaitems.append(int(child.attrib["id"]))

        borders: list[TopLevelNode] = []
        brushes: list[TopLevelNode] = []
        tilesets: list[TopLevelNode] = []
        sequence = 0

        for include_file in include_files:
            include_path = version_dir / include_file
            root_tag, elements, include_messages = parse_top_level_elements(include_path)
            if include_messages:
                issues.append(FileIssue(path=include_file, messages=include_messages))
            for element in elements:
                node = TopLevelNode(
                    source_file=include_file,
                    root_tag=root_tag,
                    tag=element.tag,
                    sequence=sequence,
                    element=element,
                )
                sequence += 1
                if element.tag == "border":
                    borders.append(node)
                elif element.tag == "brush":
                    brushes.append(node)
                elif element.tag == "tileset":
                    tilesets.append(node)

        items: list[TopLevelNode] = []
        for item_file in ITEM_REGISTRY_FILES:
            item_path = version_dir / item_file
            if not item_path.exists():
                continue
            root_tag, elements, item_messages = parse_top_level_elements(item_path)
            if item_messages:
                issues.append(FileIssue(path=item_file, messages=item_messages))
            for element in elements:
                items.append(
                    TopLevelNode(
                        source_file=item_file,
                        root_tag=root_tag,
                        tag=element.tag,
                        sequence=len(items),
                        element=element,
                    )
                )

        creatures: list[TopLevelNode] = []
        creatures_path = version_dir / "creatures.xml"
        if creatures_path.exists():
            root_tag, elements, creature_messages = parse_top_level_elements(creatures_path)
            if creature_messages:
                issues.append(FileIssue(path="creatures.xml", messages=creature_messages))
            for element in elements:
                creatures.append(
                    TopLevelNode(
                        source_file="creatures.xml",
                        root_tag=root_tag,
                        tag=element.tag,
                        sequence=len(creatures),
                        element=element,
                    )
                )

        items_otb = version_dir / "items.otb"
        return LegacyVersionData(
            version=version_dir.name,
            version_dir=version_dir,
            include_files=include_files,
            metaitems=metaitems,
            borders=borders,
            brushes=brushes,
            tilesets=tilesets,
            items=items,
            creatures=creatures,
            items_otb=items_otb if items_otb.exists() else None,
            issues=issues,
        )


class Normalizer:
    def normalize(self, legacy: LegacyVersionData) -> NormalizedVersionData:
        palette_includes: dict[str, list[str]] = {palette: [] for palette in PALETTE_ORDER}
        tilesets: list[ModularTileset] = []
        assigned_raw_item_ids: set[int] = set()
        assigned_creature_names: set[str] = set()
        path_counts: Counter[str] = Counter()

        for node in legacy.tilesets:
            tileset_name = node.element.attrib.get("name", "Unnamed")
            for wrapper_index, wrapper in enumerate(list(node.element)):
                wrapper_tag = wrapper.tag
                file_path = self._tileset_path(tileset_name, wrapper_tag, path_counts)
                entry = ModularTileset(
                    name=tileset_name,
                    wrapper=wrapper_tag,
                    file_path=file_path,
                    source_file=node.source_file,
                    sequence=node.sequence * 10 + wrapper_index,
                    children=[clone_element(child) for child in list(wrapper)],
                )
                tilesets.append(entry)

                for palette in entry.palettes:
                    palette_includes[palette].append(file_path)

                for child in wrapper:
                    if child.tag == "item":
                        assigned_raw_item_ids.update(expand_item_ids(child))
                    elif child.tag == "creature" and "name" in child.attrib:
                        assigned_creature_names.add(child.attrib["name"])

        creature_registry = [clone_element(node.element) for node in legacy.creatures if node.tag == "creature"]
        creature_names_in_order = [element.attrib["name"] for element in creature_registry if "name" in element.attrib]
        creature_type_by_name = {element.attrib["name"]: element.attrib.get("type", "") for element in creature_registry if "name" in element.attrib}
        unassigned_creatures = [name for name in creature_names_in_order if name not in assigned_creature_names]
        unassigned_npcs = [name for name in unassigned_creatures if creature_type_by_name.get(name, "").lower() == "npc"]
        unassigned_npc_set = set(unassigned_npcs)
        unassigned_other_creatures = [name for name in unassigned_creatures if name not in unassigned_npc_set]

        if unassigned_npcs:
            npc_file = self._tileset_path("NPCs", "creatures", path_counts)
            tilesets.append(
                ModularTileset(
                    name="NPCs",
                    wrapper="creatures",
                    file_path=npc_file,
                    source_file="generated",
                    sequence=max((tileset.sequence for tileset in tilesets), default=0) + 1,
                    children=[ET.Element("creature", {"name": name}) for name in unassigned_npcs],
                )
            )
            palette_includes["Creature"].append(npc_file)

        if unassigned_other_creatures:
            others_creature_file = self._tileset_path("Others", "creatures", path_counts)
            tilesets.append(
                ModularTileset(
                    name="Others",
                    wrapper="creatures",
                    file_path=others_creature_file,
                    source_file="generated",
                    sequence=max((tileset.sequence for tileset in tilesets), default=0) + 1,
                    children=[ET.Element("creature", {"name": name}) for name in unassigned_other_creatures],
                )
            )
            palette_includes["Creature"].append(others_creature_file)

        registry_item_elements = [clone_element(node.element) for node in legacy.items if node.tag == "item"]
        registry_item_ids = item_coverage(registry_item_elements)
        metaitem_ids = set(legacy.metaitems)
        unassigned_raw_ids = sorted(registry_item_ids - assigned_raw_item_ids - metaitem_ids)
        if unassigned_raw_ids:
            others_raw_file = self._tileset_path("Others", "raw", path_counts)
            tilesets.append(
                ModularTileset(
                    name="Others",
                    wrapper="raw",
                    file_path=others_raw_file,
                    source_file="generated",
                    sequence=max((tileset.sequence for tileset in tilesets), default=0) + 1,
                    children=compact_item_elements(unassigned_raw_ids),
                )
            )
            palette_includes["Raw"].append(others_raw_file)

        validation = self._build_validation(
            legacy=legacy,
            tilesets=tilesets,
            palette_includes=palette_includes,
            assigned_raw_item_ids=assigned_raw_item_ids,
            assigned_creature_names=assigned_creature_names,
            registry_item_ids=registry_item_ids,
        )

        return NormalizedVersionData(
            version=legacy.version,
            metaitems=list(legacy.metaitems),
            borders=[clone_element(node.element) for node in legacy.borders if node.tag == "border"],
            brushes=[clone_element(node.element) for node in legacy.brushes if node.tag == "brush"],
            items=registry_item_elements,
            creatures=creature_registry,
            tilesets=sorted(tilesets, key=lambda tileset: (tileset.sequence, tileset.file_path)),
            palette_includes=palette_includes,
            issues=list(legacy.issues),
            validation=validation,
            items_otb=legacy.items_otb,
        )

    def _tileset_path(self, name: str, wrapper: str, path_counts: Counter[str]) -> str:
        stem = encode_filename_component(name)
        relative = f"tilesets/{wrapper}/{stem}.xml"
        path_counts[relative] += 1
        if path_counts[relative] == 1:
            return relative
        return f"tilesets/{wrapper}/{stem}_{path_counts[relative]}.xml"

    def _build_validation(
        self,
        legacy: LegacyVersionData,
        tilesets: list[ModularTileset],
        palette_includes: dict[str, list[str]],
        assigned_raw_item_ids: set[int],
        assigned_creature_names: set[str],
        registry_item_ids: set[int],
    ) -> dict[str, object]:
        legacy_tileset_wrappers: Counter[str] = Counter()
        for node in legacy.tilesets:
            for wrapper in list(node.element):
                legacy_tileset_wrappers[wrapper.tag] += 1

        normalized_tileset_wrappers = Counter(tileset.wrapper for tileset in tilesets)
        explicit_creature_tilesets = legacy_tileset_wrappers.get("creatures", 0)
        validation_messages: list[str] = []
        if explicit_creature_tilesets == 0:
            validation_messages.append("created fallback creature tilesets because the legacy data had no explicit creature palette")
        if legacy.items_otb is None:
            validation_messages.append("items.otb missing in source version")

        brush_names = {node.element.attrib.get("name", "") for node in legacy.brushes}
        border_ids = {node.element.attrib.get("id", "") for node in legacy.borders}
        creature_names = {node.element.attrib.get("name", "") for node in legacy.creatures}

        unresolved = {
            "tileset_brush_references": [],
            "tileset_creature_references": [],
            "tileset_item_references": [],
            "brush_border_references": [],
            "brush_name_references": [],
        }

        for tileset in tilesets:
            for child in tileset.children:
                if child.tag == "brush":
                    name = child.attrib.get("name", "")
                    if name and name not in brush_names:
                        unresolved["tileset_brush_references"].append({"tileset": tileset.name, "brush": name})
                elif child.tag == "creature":
                    name = child.attrib.get("name", "")
                    if name and name not in creature_names:
                        unresolved["tileset_creature_references"].append({"tileset": tileset.name, "creature": name})
                elif child.tag == "item":
                    missing = [item_id for item_id in expand_item_ids(child) if item_id not in registry_item_ids]
                    if missing:
                        unresolved["tileset_item_references"].append({"tileset": tileset.name, "missing_ids": missing[:20]})

        for node in legacy.brushes:
            brush = node.element
            for descendant in brush.iter():
                if descendant.tag in {"border", "match_border", "replace_border"}:
                    border_id = descendant.attrib.get("id")
                    if border_id and border_id not in border_ids:
                        unresolved["brush_border_references"].append(
                            {"brush": brush.attrib.get("name", ""), "border_id": border_id, "tag": descendant.tag}
                        )
                if descendant.tag in {"friend", "enemy"}:
                    name = descendant.attrib.get("name", "")
                    if name and name not in brush_names and name not in {"all", "none"}:
                        unresolved["brush_name_references"].append(
                            {"brush": brush.attrib.get("name", ""), "reference": name, "tag": descendant.tag}
                        )
                if descendant.tag == "border":
                    target = descendant.attrib.get("to")
                    if target and target not in {"all", "none"} and target not in brush_names:
                        unresolved["brush_name_references"].append(
                            {"brush": brush.attrib.get("name", ""), "reference": target, "tag": "border.to"}
                        )

        return {
            "legacy_counts": {
                "borders": len(legacy.borders),
                "brushes": len(legacy.brushes),
                "metaitems": len(legacy.metaitems),
                "creatures": len(legacy.creatures),
                "items": len(legacy.items),
                "item_coverage": len(registry_item_ids),
                "tilesets_by_wrapper": dict(legacy_tileset_wrappers),
            },
            "normalized_counts": {
                "borders": len(legacy.borders),
                "brushes": len(legacy.brushes),
                "metaitems": len(legacy.metaitems),
                "creatures": len(legacy.creatures),
                "items": len(legacy.items),
                "item_coverage": len(registry_item_ids),
                "tilesets_by_wrapper": dict(normalized_tileset_wrappers),
                "palettes": {palette: len(paths) for palette, paths in palette_includes.items()},
                "assigned_raw_items": len(assigned_raw_item_ids),
                "assigned_creatures": len(assigned_creature_names),
            },
            "messages": validation_messages,
            "unresolved": unresolved,
        }


class ModularWriter:
    def __init__(self, output_root: Path) -> None:
        self.output_root = output_root

    def write(self, version: NormalizedVersionData) -> Path:
        version_dir = self.output_root / version.version
        if version_dir.exists():
            shutil.rmtree(version_dir)
        version_dir.mkdir(parents=True, exist_ok=True)

        for folder in ("borders", "brushes", "creatures", "items", "tilesets"):
            (version_dir / folder).mkdir(parents=True, exist_ok=True)

        self._write_materials(version_dir, version.metaitems)
        self._write_palettes(version_dir, version.palette_includes)
        self._write_border_module(version_dir, version.borders)
        self._write_brush_module(version_dir, version.brushes)
        self._write_creature_module(version_dir, version.creatures)
        self._write_item_module(version_dir, version.items)
        self._write_tilesets(version_dir, version.tilesets)

        if version.items_otb is not None and version.items_otb.exists():
            shutil.copy2(version.items_otb, version_dir / "items.otb")

        report = {
            "version": version.version,
            "issues": [{"path": issue.path, "messages": issue.messages} for issue in version.issues],
            "validation": version.validation,
            "generated_files": self._generated_file_list(version_dir),
        }
        report["smoke_test"] = self._smoke_test(version_dir)
        (version_dir / "conversion_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        return version_dir

    def _write_materials(self, version_dir: Path, metaitems: list[int]) -> None:
        root = ET.Element("materials")
        for metaitem in metaitems:
            ET.SubElement(root, "metaitem", {"id": str(metaitem)})

        for section_name, folder, recursive in MODULE_SECTIONS:
            section = ET.SubElement(root, section_name)
            include_attrib = {"folder": folder}
            if recursive:
                include_attrib["subfolders"] = "true"
            ET.SubElement(section, "include", include_attrib)

        palettes = ET.SubElement(root, "palettes")
        ET.SubElement(palettes, "include", {"file": "palettes.xml"})
        write_xml(version_dir / "materials.xml", root)

    def _write_palettes(self, version_dir: Path, palette_includes: dict[str, list[str]]) -> None:
        root = ET.Element("palettes")
        for palette_name in PALETTE_ORDER:
            palette = ET.SubElement(root, "palette", {"name": palette_name})
            tileset = ET.SubElement(palette, "tileset")
            for file_path in palette_includes[palette_name]:
                ET.SubElement(tileset, "include", {"file": file_path.replace("\\", "/")})
        write_xml(version_dir / "palettes.xml", root)

    def _write_border_module(self, version_dir: Path, borders: list[ET.Element]) -> None:
        root = ET.Element("materials")
        for border in borders:
            root.append(clone_element(border))
        write_xml(version_dir / "borders" / "borders.xml", root)

    def _write_brush_module(self, version_dir: Path, brushes: list[ET.Element]) -> None:
        root = ET.Element("brushes")
        for brush in brushes:
            root.append(clone_element(brush))
        write_xml(version_dir / "brushes" / "brushes.xml", root)

    def _write_creature_module(self, version_dir: Path, creatures: list[ET.Element]) -> None:
        root = ET.Element("creatures")
        for creature in creatures:
            root.append(clone_element(creature))
        write_xml(version_dir / "creatures" / "creatures.xml", root)

    def _write_item_module(self, version_dir: Path, items: list[ET.Element]) -> None:
        root = ET.Element("items")
        for item in items:
            root.append(clone_element(item))
        write_xml(version_dir / "items" / "items.xml", root)

    def _write_tilesets(self, version_dir: Path, tilesets: list[ModularTileset]) -> None:
        for tileset in tilesets:
            path = version_dir / tileset.file_path
            path.parent.mkdir(parents=True, exist_ok=True)
            root = ET.Element("tileset", {"name": tileset.name})
            for child in tileset.children:
                root.append(clone_element(child))
            write_xml(path, root)

    def _generated_file_list(self, version_dir: Path) -> list[str]:
        return sorted(str(path.relative_to(version_dir)).replace("\\", "/") for path in version_dir.rglob("*") if path.is_file())

    def _smoke_test(self, version_dir: Path) -> dict[str, object]:
        parsed_files: list[str] = []
        failures: list[str] = []
        for xml_path in sorted(version_dir.rglob("*.xml")):
            try:
                parse_root(xml_path)
                parsed_files.append(str(xml_path.relative_to(version_dir)).replace("\\", "/"))
            except Exception as exc:
                failures.append(f"{xml_path.name}: {exc}")
        return {"parsed_files": parsed_files, "failures": failures}


def summarize_issues(issues: list[FileIssue]) -> list[str]:
    summary: list[str] = []
    for issue in issues:
        for message in issue.messages:
            summary.append(f"{issue.path}: {message}")
    return summary


def run_conversion(base_dir: Path, output_root: Path, versions: list[str] | None) -> list[tuple[str, Path, list[str]]]:
    reader = LegacyVersionReader(base_dir)
    normalizer = Normalizer()
    writer = ModularWriter(output_root)
    selected = reader.discover_versions()
    if versions:
        version_set = set(versions)
        selected = [path for path in selected if path.name in version_set]

    results: list[tuple[str, Path, list[str]]] = []
    for version_dir in selected:
        legacy = reader.read(version_dir)
        normalized = normalizer.normalize(legacy)
        written_dir = writer.write(normalized)
        results.append((version_dir.name, written_dir, summarize_issues(normalized.issues)))
    return results


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Convert legacy RME data folders into modular XML output.")
    parser.add_argument(
        "--base-dir",
        type=Path,
        default=Path(__file__).resolve().parent,
        help="Directory that contains the numeric version folders.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "new_data",
        help="Output directory for modularized client data.",
    )
    parser.add_argument(
        "--versions",
        nargs="*",
        default=None,
        help="Optional list of version folders to convert. Defaults to all numeric folders.",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    try:
        results = run_conversion(args.base_dir, args.output_dir, args.versions)
    except Exception as exc:
        print(f"Conversion failed: {exc}", file=sys.stderr)
        return 1

    print(f"Converted {len(results)} version folder(s) into {args.output_dir}")
    for version, written_dir, issues in results:
        print(f"- {version}: {written_dir}")
        if issues:
            print(f"  issues: {len(issues)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
