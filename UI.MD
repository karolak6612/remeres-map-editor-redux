You're right, sorry! Here's the combined skill for wxWidgets:

---
name: wxwidgets-ux-ui
description: Build production-grade wxWidgets UX/UI (flows + components) with strong aesthetics, accessibility, and engineering quality. Includes anti-Codex UI enforcement.
version: 1.0.0
---

# wxWidgets UX/UI

## Purpose
Ship production-grade native desktop experiences that are visually intentional, accessible, resilient under real-world conditions, and easy to maintain — without falling into default AI aesthetic patterns.

## Auto Trigger
Activate automatically when requests include:
- UI/UX keywords: redesign, dialog, panel, toolbar, form UX, accessibility, responsive sizing, polish, visual refresh
- Interaction/state keywords: loading, empty state, error state, success state, validation, progress, layout
- wxWidgets implementation asks in C++ repositories
- Folder/file signals: files referencing wx headers, CMakeLists with wxWidgets find_package, frame/dialog/panel class hierarchies

## When To Use
Use this skill when the request includes any of the following:
- New frame, dialog, panel, or control implementation
- UX redesign of existing windows or modules
- Interaction polish (forms, menus, toolbars, state transitions)
- UI bugfixes with usability impact
- Converting rough product requirements into concrete UI contracts
- wxWidgets C++ delivery

## When NOT To Use
- Backend-only changes with no user-facing surface
- Pure refactors with no UX behavior changes
- One-off content edits with no layout or interaction impact
- Build system, CI/CD, or non-UI tooling tasks
- Design-only brainstorming when implementation is explicitly out of scope

## Stack Defaults
Assume these unless repo evidence says otherwise:
- wxWidgets latest stable
- C++ with strict typing
- Native platform controls preferred over custom-drawn equivalents
- Existing local sizing/spacing conventions over new patterns

## Operating Rules
1. Repo-first, not blank-canvas: inspect the codebase before proposing UI.
2. Minimize churn: edit only the smallest set of files needed.
3. Preserve design conventions and product language already present.
4. Prefer clear contracts over implicit behavior.
5. Native controls used correctly beat custom-drawn controls used decoratively every time.

---

## Anti-Codex UI Enforcement (Must Follow)

In your internal reasoning, list every default AI UI move you would normally make — then do none of them. Pick the harder, cleaner option every time.

### Keep It Normal

- **Panels:** standard wxPanel with default background, simple border or none, no decorative nested shells
- **Sizers:** wxBoxSizer and wxGridSizer used straightforwardly, consistent gaps, predictable structure
- **StaticText:** plain labels, correct font size hierarchy, no uppercase eyebrow labels, no decorative copy explaining what the UI does
- **Buttons:** standard wxButton, no custom-drawn gradient fills, no pill shapes, no oversized padding
- **TextCtrl:** standard border, simple focus behavior, no animated or morphing custom styles
- **ListCtrl / TreeCtrl:** clean rows, simple column headers, subtle selection highlight, left-aligned text, no badge overload
- **Notebook / Tabs:** simple tab strip, no custom-drawn sliding indicators, no pill tab backgrounds
- **Dialogs:** centered, standard title bar, straightforward OK/Cancel placement, no decorative inner panels
- **Toolbars:** wxToolBar at standard height, clear icons, no decorative separators or custom backgrounds
- **StatusBar:** minimal, functional, no decorative meta text
- **Menus:** standard system menus, no custom-drawn items unless strictly necessary
- **Splitters:** wxSplitterWindow used when layout genuinely needs it, not as a decorative device
- **Icons:** consistent size, monochrome or simple color, no decorative backgrounds, no glows
- **Typography:** system default font, scale via wxFont point sizes only when hierarchy demands it, no mixed font families for decoration
- **Spacing:** consistent multiples of a base unit, no random gaps, no excessive padding
- **Borders:** wx border flags used functionally, no thick decorative borders, no gradient borders
- **Sizing:** SetMinSize and Fit used correctly, no hardcoded oversized dimensions to fake spaciousness

Think FileZilla. Think KiCad. Think Audacity. They don't try to grab attention. They just work.

### Hard No
- No custom-drawn gradient backgrounds on panels as default visual language
- No oversized padding creating dead space to look expensive
- No decorative StaticText blocks explaining what a panel does
- No eyebrow labels — no small uppercase StaticText used as section titles
- No hero-style introductory sections inside dialogs or tool windows
- No pill-shaped custom-drawn buttons unless the product explicitly requires it
- No glassmorphism or frosted-panel effects via custom drawing
- No colored glow effects around focused controls
- No dramatic custom drop shadows painted onto panels
- No purely decorative nested wxPanel shells creating a fake "card" look
- No metric-card grid as the first instinct for any summary view
- No fake progress bars or charts that exist only to fill space
- No ornamental labels unless they come from the actual product voice
- No generic descriptive copy inside UI panels
- No bounce or overshoot in custom animations
- No decorative status dots unless genuinely functional
- No mixed font families used to fake a premium feel
- No right-side rail panels added by default without a real architectural reason
- No footer StaticText lines with meta information about the layout
- No KPI card grids as a default dashboard composition
- No badge counts on every list item or tree node unless the count is meaningful

### Specifically Banned Patterns
- Repeating heavy custom-drawn rounded rectangles across every panel, button, and card
- Sidebar panels significantly wider than needed with a large brand block on top
- Custom-drawn charts placed in decorative panel shells with no product reason
- Donut or pie charts paired with vague percentages just to fill a summary area
- Mixed alignment where some content is left-aligned and other content floats center for no reason
- Overuse of muted gray text that weakens contrast and readability
- Dark background plus bright accent colors applied without restraint
- Typography that feels like a copied template rather than belonging to the product
- Decorative section notes inside panels explaining what each section is for
- Dramatic shadow values on custom-drawn controls
- Gradient fills on progress or pipeline bars used purely for decoration
- Multiple nested panel types stacked just to create visual depth

---

## Required UI States (Enforced)
Every feature and screen must define and implement:
- Loading / busy
- Empty
- Error (recoverable and fatal where applicable)
- Success / confirmation
- Disabled/pending action states
- Partial/degraded data states

---

## Non-Negotiables
1. **Accessibility:** keyboard usability, logical tab order, proper labels on all controls, visible focus.
2. **Resize behavior:** windows and dialogs must resize gracefully using sizers, validated at small and large sizes.
3. **Stability:** avoid layout shift; use reserved space and consistent sizer proportions.
4. **Engineering quality:** typed props, explicit variants, predictable state transitions.
5. **Minimal churn:** avoid unrelated rewrites or design convention drift.
6. **Verification required:** run practical checks before claiming completion.

---

## Design System Awareness (Must Do)

### Discovery Order
1. Locate existing control conventions, spacing patterns, and font usage in the repo.
2. Trace data loading and mutation paths for the affected flow.
3. Identify existing state patterns (progress dialogs, inline errors, empty list handling).
4. Identify existing accessibility patterns (tab order, label associations, keyboard shortcuts).

### Minimum Context to Gather
- Existing visual language (font sizes, spacing rhythm, border usage)
- Existing state patterns (busy cursors, inline errors, empty screens)
- Existing accessibility patterns (tab order conventions, accelerator keys)
- Existing verification standards in the repo

---

## Workflow (Must Follow)

### Step 0: Repo-First Discovery
Establish context before proposing UI changes:
- Confirm window/dialog/panel ownership and component boundaries.
- Confirm spacing, font, and sizing patterns to reuse.
- Confirm data and async/event flows plus threading boundaries.

### Step 1: UX Brief
Produce a short implementation brief before coding:
- User persona and job-to-be-done
- Screen(s) and user goal
- Success criteria (observable outcomes)
- Technical constraints
- Risk list for edge conditions

### Step 2: Information Architecture + States
Define:
- Layout regions and information hierarchy
- Primary interaction paths
- Full states matrix (see template below)

### Step 3: Visual Direction
Commit to concrete visual choices aligned with the repo:
- Font hierarchy and density
- Spacing cadence
- Surface and border language
- Color and contrast policy (see palette priority below)
- Motion policy — prefer none or minimal; use wxBusyInfo or progress dialogs for long operations

Rule: describe decisions as implementable sizer/font/color values, not adjectives.

### Step 4: Component Contracts
For each window, dialog, or panel module define:
- Constructor parameters and public interface
- Variants and default behavior
- Events/callbacks and side effects
- Accessibility constraints (tab order, accelerator keys, label associations)
- State handling rules
- Error/fallback behavior if applicable

### Step 5: File Plan
Create a minimal blast-radius file plan:
- Files to create
- Files to edit
- Files explicitly not touched
- Migration notes if replacing legacy UI

### Step 6: Implementation
Build in small, testable increments:
- Shared primitives and base panels first
- Composite sections second
- Wiring and event logic third
- Polish and microcopy last

Implementation guardrails:
- Prefer composition via sizer nesting over monolithic panels.
- Reuse existing control patterns when compatible.
- Handle disabled, pending, and failed actions explicitly.
- Use wxValidator or equivalent for form validation.

### Step 7: QA Gates
Required functional QA:
- Keyboard navigation path works end-to-end.
- Tab order is logical and deterministic across dialogs and panels.
- All states render correctly (loading/empty/error/success).
- Resize behavior is correct at common window sizes.
- Long text does not break layout.
- Permission-denied or unavailable states provide recovery path or explanation.

Required technical QA:
- Compiles cleanly with no warnings.
- Relevant tests pass or manual test script is documented.
- No unrelated files changed.

### Step 8: Summary
Produce a concise summary:
- What changed
- Why this approach
- UX behavior by state
- Accessibility considerations
- Verification evidence
- Risks and follow-ups

---

## Output Contract
Use this exact required response section order:
1. UX Brief
2. UX States Matrix
3. Visual Direction
4. Component Contracts
5. File Plan
6. Implementation
7. Verification
8.Summary

---

## Color Priority
When selecting colors, follow this order strictly:

1. **Highest priority:** Use existing colors from the user's project if provided.
2. If no project palette exists, **draw inspiration from one of the predefined palettes below**.
3. **Do not invent random color combinations** unless explicitly requested.

### Dark Palettes

| Palette | Background | Surface | Primary | Secondary | Accent | Text |
|---|---|---|---|---|---|---|
| Midnight Canvas | `#0a0e27` | `#151b3d` | `#6c8eff` | `#a78bfa` | `#f472b6` | `#e2e8f0` |
| Obsidian Depth | `#0f0f0f` | `#1a1a1a` | `#00d4aa` | `#00a3cc` | `#ff6b9d` | `#f5f5f5` |
| Slate Noir | `#0f172a` | `#1e293b` | `#38bdf8` | `#818cf8` | `#fb923c` | `#f1f5f9` |
| Carbon Elegance | `#121212` | `#1e1e1e` | `#bb86fc` | `#03dac6` | `#cf6679` | `#e1e1e1` |
| Deep Ocean | `#001e3c` | `#0a2744` | `#4fc3f7` | `#29b6f6` | `#ffa726` | `#eceff1` |
| Charcoal Studio | `#1c1c1e` | `#2c2c2e` | `#0a84ff` | `#5e5ce6` | `#ff375f` | `#f2f2f7` |
| Graphite Pro | `#18181b` | `#27272a` | `#a855f7` | `#ec4899` | `#14b8a6` | `#fafafa` |
| Void Space | `#0d1117` | `#161b22` | `#58a6ff` | `#79c0ff` | `#f78166` | `#c9d1d9` |
| Twilight Mist | `#1a1625` | `#2d2438` | `#9d7cd8` | `#7aa2f7` | `#ff9e64` | `#dcd7e8` |
| Onyx Matrix | `#0e0e10` | `#1c1c21` | `#00ff9f` | `#00e0ff` | `#ff0080` | `#f0f0f0` |

### Light Palettes

| Palette | Background | Surface | Primary | Secondary | Accent | Text |
|---|---|---|---|---|---|---|
| Cloud Canvas | `#fafafa` | `#ffffff` | `#2563eb` | `#7c3aed` | `#dc2626` | `#0f172a` |
| Pearl Minimal | `#f8f9fa` | `#ffffff` | `#0066cc` | `#6610f2` | `#ff6b35` | `#212529` |
| Ivory Studio | `#f5f5f4` | `#fafaf9` | `#0891b2` | `#06b6d4` | `#f59e0b` | `#1c1917` |
| Linen Soft | `#fef7f0` | `#fffbf5` | `#d97706` | `#ea580c` | `#0284c7` | `#292524` |
| Porcelain Clean | `#f9fafb` | `#ffffff` | `#4f46e5` | `#8b5cf6` | `#ec4899` | `#111827` |
| Cream Elegance | `#fefce8` | `#fefce8` | `#65a30d` | `#84cc16` | `#f97316` | `#365314` |
| Arctic Breeze | `#f0f9ff` | `#f8fafc` | `#0284c7` | `#0ea5e9` | `#f43f5e` | `#0c4a6e` |
| Alabaster Pure | `#fcfcfc` | `#ffffff` | `#1d4ed8` | `#2563eb` | `#dc2626` | `#1e293b` |
| Sand Warm | `#faf8f5` | `#ffffff` | `#b45309` | `#d97706` | `#059669` | `#451a03` |
| Frost Bright | `#f1f5f9` | `#f8fafc` | `#0f766e` | `#14b8a6` | `#e11d48` | `#0f172a` |

---

## Edge-Case Playbook

### Long Text
- Support wrapping where readability matters.
- Use truncation only where density matters and provide access to full value via tooltip or expand.
- Stress-test with long strings, translated text, and large numbers.

### Slow Operations
- Show wxProgressDialog or wxBusyInfo immediately for operations over ~300ms.
- Keep primary controls in a predictable state while operations run.
- Distinguish initial load from background refresh.

### Permission Constraints
- Separate not-found from forbidden states.
- Explain why actions are unavailable.
- Provide a next best action where possible.

### Partial Failures
- Render available data.
- Isolate failed subsections with local retry where possible.
- Preserve user progress where possible.

---

## Accessibility Baseline
- Use native wxWidgets controls first; do not replace them with custom-drawn equivalents without strong reason.
- Associate StaticText labels with their controls via correct sizer proximity and accelerator keys.
- Ensure logical tab order via SetTabOrder or sizer construction order.
- Provide keyboard shortcuts for primary actions.
- Ensure all interactive controls have descriptive labels for screen reader compatibility.

---

## Minimal-Churn Rules
- Do not rename or move files without clear benefit.
- Do not replace existing controls unless current behavior blocks requirements.
- Keep public class interfaces backward compatible when possible.
- Separate cosmetic cleanup from behavior changes.

---

## Practical Heuristics To Avoid Generic AI UI
1. Choose one strong structural idea per screen, not many weak flourishes.
2. Make hierarchy obvious in 3 seconds: title, status, primary action.
3. Use purposeful density based on task frequency.
4. Limit visual motifs; repeat them consistently.
5. Prefer high-signal microcopy tied to user outcome.
6. Promote one primary action and demote secondary actions.
7. Use motion only to communicate busy state or progress — nothing decorative.
8. Keep ornamental styling secondary to readability and speed.

