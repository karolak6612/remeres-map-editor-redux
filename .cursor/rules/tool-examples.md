# Tool Input Examples — Selection Guidance

## Purpose

Improve tool selection accuracy by providing concrete input examples for the most-used tools. When choosing which tool to use, reference these examples to match the current task to the correct tool.

## Tool Examples

### git — Version Control
Use for repository state, history, and branch management.
- **Check changes:** `git diff --stat` — file-level summary of uncommitted changes
- **Recent history:** `git log --oneline -10` — last 10 commits with conventional messages
- **Branch comparison:** `git diff main...HEAD --stat` — all changes since branching from main

### github-cli — GitHub Operations
Use for PRs, issues, and repository management.
- **Create PR:** `gh pr create --title 'feat: ...' --body '## Summary...'`
- **List issues:** `gh issue list --state open --label bug`
- **PR status:** `gh pr view 123 --json reviews,statusCheckRollup`
