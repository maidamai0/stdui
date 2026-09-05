# Better Review Workflow for Design Documents

## The Problem You Identified

**Current workflow is clumsy:**
- Multiple files just for one review (pollutes repo)
- Inline comments pollute the design doc
- No good local PR-like experience

## Better Approaches

### Option 1: Git + Pull Request (Recommended)

**Workflow:**
```bash
# Create branch for design doc
git checkout -b design/rendering-phase4

# Write design doc
vim docs/rendering-design.md
git add docs/rendering-design.md
git commit -m "docs: Phase 4 rendering design"

# Push and create PR
git push -u origin design/rendering-phase4
# Create PR on GitHub/GitLab

# Review happens in PR interface
# - Line-by-line comments
# - Thread conversations
# - Approve/request changes

# After approval, merge to main
```

**Benefits:**
- ✅ GitHub/GitLab PR interface (familiar, powerful)
- ✅ Line-by-line comments with threading
- ✅ Review history preserved in PR
- ✅ No pollution of docs/ folder
- ✅ Clean merge when done

**This is what most teams do for design reviews.**

---

### Option 2: Review Tool (if no Git hosting)

If you don't want to use GitHub PRs:

**Tools:**
- **Gerrit** - Code review tool, can review any text file
- **Review Board** - Similar to Gerrit
- **VSCode + GitLens** - Local diff review with comments

---

### Option 3: Markdown Comment Syntax (Lightweight)

If you want to keep it simple and local:

**Design doc with hidden review sections:**
```markdown
# Rendering Design

## Section 1: Architecture

Content here...

<!-- REVIEW THREAD 1
Reviewer: What about X?
Author: Good point, changed to Y.
Status: RESOLVED
-->

## Section 2: Implementation
...
```

**Or use HTML comments that don't render:**
```markdown
Text here

<!-- TODO(reviewer-name): Need to clarify this section -->
```

**Process:**
1. Write design with `<!-- TODO: ... -->` markers
2. Reviewer adds `<!-- REVIEW: ... -->` comments
3. Author addresses, marks `<!-- RESOLVED: ... -->`
4. Clean up comments before merge

---

## Recommendation for Your Project

**Use GitHub Pull Requests** for design doc reviews:

1. **One-time setup:**
   ```bash
   # Make sure you have GitHub repo set up
   git remote -v  # Should show your GitHub repo
   ```

2. **For each design doc:**
   ```bash
   git checkout -b design/phase4-rendering
   # Write docs/rendering-design-v2.md
   git add docs/rendering-design-v2.md
   git commit -m "docs: Phase 4 rendering architecture"
   git push -u origin design/phase4-rendering
   # Create PR on GitHub
   ```

3. **Review in GitHub UI:**
   - See the full diff
   - Comment on specific lines
   - Have threaded discussions
   - Mark "Request Changes" or "Approve"

4. **After approval:**
   ```bash
   git checkout main
   git merge design/phase4-rendering
   git push
   ```

**Benefits:**
- No extra files in repo
- Professional workflow
- Review history preserved
- Easy to reference later ("see PR #42 for context")

---

## What Should Go in `.reviews/` Folder

**Only temporary scratch notes:**
- Personal review notes
- Comparison tables
- Draft feedback before posting to PR

**Not:**
- Final design docs (those go in `docs/`)
- Review comments (those go in PR)

---

## For This Specific Review

Since we're mid-review, let's finish this one with the current approach, then:

**Going forward:**
1. I create design doc in `docs/`
2. Push to branch, create PR
3. You review in GitHub PR interface
4. I address comments in PR
5. You approve
6. Merge to main

**Much cleaner!**

Would you like me to:
1. Set this up now (create PR for the rendering design)?
2. Or continue with current approach and switch to PRs for the next design doc?
