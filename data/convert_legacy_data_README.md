# Legacy Data XML Converter

`convert_legacy_data.py` converts legacy Remere's Map Editor client data folders into the modular XML layout used by `new_data/`.

The script is intended to run from this `data/` directory. It scans numeric client folders such as `740`, `800`, `860`, and `1098`, then writes converted output to `new_data/<client_version>/`.

## Old XML Structure

Legacy client folders are version-specific and not fully uniform. Each folder contains `items.otb`, `items.xml`, `creatures.xml`, `materials.xml`, and several material XML files.

Typical examples:

```text
800/
  borders.xml
  collections.xml
  creature_palette.xml
  creatures.xml
  doodads.xml
  grounds.xml
  item_palette.xml
  items.otb
  items.xml
  materials.xml
  raw_palette.xml
  tilesets.xml
  walls.xml
  walls_extra.xml
```

```text
1098/
  borders.xml
  creatures.xml
  doodads.xml
  grounds.xml
  items.otb
  items.xml
  materials.xml
  tilesets.xml
  walls.xml
```

### Legacy Entry Point

`materials.xml` is the legacy material manifest. It contains ordered includes and, in many versions, editor-only metaitems:

```xml
<materials>
    <metaitem id="80"/>
    <include file="borders.xml"/>
    <include file="grounds.xml"/>
    <include file="walls.xml"/>
    <include file="doodads.xml"/>
    <include file="tilesets.xml"/>
</materials>
```

The include order matters. The converter preserves that order when collecting borders, brushes, and tilesets.

### Legacy Responsibilities

Legacy files are coupled and often mix different concepts:

| File | Common role |
|---|---|
| `borders.xml` | Defines `<border>` entries and `<borderitem>` edge mappings. |
| `grounds.xml` | Defines ground brushes, border behavior, friends/enemies, and special ground logic. |
| `walls.xml` | Defines wall brushes and can also define wall-related tilesets. |
| `walls_extra.xml` | Defines additional wall detail and archway doodad brushes in `760` and `800`. |
| `doodads.xml` | Defines doodad/table/carpet brushes and, in `760` and `800`, embedded doodad tilesets. |
| `tilesets.xml` | Defines user-facing tileset groupings. Newer versions consolidate most palette structure here. |
| `creature_palette.xml` | Defines creature tilesets in `760` and `800`. Other versions do not have explicit creature palette files. |
| `item_palette.xml` | Defines item palette tilesets in `760` and `800`. |
| `raw_palette.xml` | Defines raw item tilesets in `760` and `800`. |
| `collections.xml` | Defines collection palette tilesets in `760` and `800`. |
| `items.xml` | Raw item registry. `740` also has `items2.xml`, which is merged after `items.xml`. |
| `creatures.xml` | Raw creature registry. |

### Legacy Tileset Wrappers

Legacy `<tileset>` nodes contain category wrapper nodes. These wrappers decide which palette category receives the entries:

```xml
<tileset name="Nature">
    <terrain>
        <brush name="grass"/>
        <brush name="sand"/>
    </terrain>
    <doodad>
        <brush name="trees"/>
    </doodad>
    <raw>
        <item fromid="1285" toid="1359"/>
    </raw>
</tileset>
```

Common wrapper names:

| Wrapper | Target palette |
|---|---|
| `terrain` | Terrain |
| `doodad` | Doodad |
| `collections` | Collection |
| `items` | Item |
| `creatures` | Creature |
| `raw` | Raw |
| `terrain_and_raw` | Terrain and Raw |
| `collections_and_terrain` | Collection and Terrain |
| `doodad_and_raw` | Doodad and Raw |
| `items_and_raw` | Item and Raw |

The converter handles hybrid wrapper names generically by splitting on `_and_`.

## New XML Structure

Converted folders are written to `new_data/<client_version>/`.

Example:

```text
new_data/800/
  borders/
    borders.xml
  brushes/
    brushes.xml
  creatures/
    creatures.xml
  items/
    items.xml
  tilesets/
    collections/
    collections_and_terrain/
    creatures/
    doodad/
    doodad_and_raw/
    items/
    items_and_raw/
    raw/
    terrain/
  conversion_report.json
  items.otb
  materials.xml
  palettes.xml
```

### New Entry Point

`materials.xml` becomes a modular manifest:

```xml
<materials>
    <borders>
        <include folder="borders/" />
    </borders>
    <brushes>
        <include folder="brushes/" />
    </brushes>
    <creatures>
        <include folder="creatures/" />
    </creatures>
    <items>
        <include folder="items/" />
    </items>
    <tilesets>
        <include folder="tilesets/" subfolders="true" />
    </tilesets>
    <palettes>
        <include file="palettes.xml" />
    </palettes>
</materials>
```

Metaitems from legacy `materials.xml` are preserved at the top of the generated `materials.xml`.

### Modular Files

| Output path | Contents |
|---|---|
| `borders/borders.xml` | All legacy `<border>` definitions. |
| `brushes/brushes.xml` | All legacy `<brush>` definitions from material include files. |
| `creatures/creatures.xml` | Raw creature registry from legacy `creatures.xml`. |
| `items/items.xml` | Raw item registry from legacy `items.xml` and, for `740`, `items2.xml`. |
| `tilesets/<category>/<tileset name>.xml` | One modular tileset per legacy tileset wrapper. |
| `palettes.xml` | Top-level palette layout with explicit file includes. |
| `conversion_report.json` | Counts, generated files, smoke-test results, repaired input notes, and unresolved legacy references. |

Tileset file names use the visible tileset name only. Category suffixes such as `_raw` or `_doodad` are not appended. Slashes in tileset names are replaced with spaces.

If two generated files would have the same path in the same folder, the later file receives a numeric suffix such as `_2`.

### Generated Fallback Tilesets

Some legacy versions do not explicitly define creature palette tilesets. In those cases, the converter creates:

```text
tilesets/creatures/Others.xml
```

If the creature registry contains NPC entries, it also creates:

```text
tilesets/creatures/NPCs.xml
```

The converter also creates:

```text
tilesets/raw/Others.xml
```

for raw item ids from `items.xml` that were not assigned to any explicit raw-capable legacy tileset.

## Script Usage

Run from `data/`:

```powershell
python .\convert_legacy_data.py
```

This converts every numeric client folder and writes output to:

```text
data/new_data/
```

Convert only selected versions:

```powershell
python .\convert_legacy_data.py --versions 740 800 1098
```

Use a custom source directory:

```powershell
python .\convert_legacy_data.py --base-dir C:\path\to\data
```

Use a custom output directory:

```powershell
python .\convert_legacy_data.py --output-dir C:\path\to\new_data
```

Combine options:

```powershell
python .\convert_legacy_data.py --base-dir C:\path\to\data --output-dir C:\path\to\converted --versions 800 1098
```

## Validation

The converter validates generated output as part of each run:

1. It writes `conversion_report.json` for every converted version.
2. It smoke-parses every generated XML file.
3. It records source repairs, such as removed NUL bytes, fallback `cp1252` decoding, or escaped malformed ampersands.
4. It records unresolved legacy references without stopping conversion.

To compile-check the script:

```powershell
python -m py_compile .\convert_legacy_data.py
```

To inspect whether a converted version had XML smoke-test failures, open:

```text
new_data/<client_version>/conversion_report.json
```

and check:

```json
"smoke_test": {
  "failures": []
}
```

## Conversion Notes

The converter preserves semantic XML content and declaration order, not byte-for-byte formatting.

The script intentionally targets the modular loader architecture represented by `new_data/`. It does not rewrite the C++ loader and does not make the modular output compatible with older legacy-only material loading code.

Known source data issues are repaired only enough to parse and preserve the XML structure:

| Source issue | Repair |
|---|---|
| NUL bytes in XML files | Removed before parsing. |
| Non-UTF-8 text in XML files | Decoded with `cp1252` fallback. |
| Bare `&` in attribute values | Escaped to `&amp;`. |
| Invalid XML control characters | Removed before parsing. |

The report file is the source of truth for what the converter repaired or could not resolve for each version.
