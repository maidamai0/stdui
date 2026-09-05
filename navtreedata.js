/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Standard UI", "index.html", [
    [ "stdui", "index.html", "index" ],
    [ "Rendering Subsystem: Research Report & Design Proposal", "md_docs_2rendering-research-report.html", [
      [ "Executive Summary", "md_docs_2rendering-research-report.html#autotoc_md60", null ],
      [ "1. Context & Requirements", "md_docs_2rendering-research-report.html#autotoc_md62", [
        [ "1.1 Current State", "md_docs_2rendering-research-report.html#autotoc_md63", null ],
        [ "1.2 Target Use Case", "md_docs_2rendering-research-report.html#autotoc_md64", null ],
        [ "1.3 Design Constraints from Architecture", "md_docs_2rendering-research-report.html#autotoc_md65", null ]
      ] ],
      [ "2. Rendering Architecture Research", "md_docs_2rendering-research-report.html#autotoc_md67", [
        [ "2.1 Rendering Tiers Analysis", "md_docs_2rendering-research-report.html#autotoc_md68", [
          [ "Tier 1: UI Chrome (Lightweight)", "md_docs_2rendering-research-report.html#autotoc_md69", null ],
          [ "Tier 2: Main Viewport (Heavy)", "md_docs_2rendering-research-report.html#autotoc_md70", null ]
        ] ],
        [ "2.2 Backend Abstraction Strategy", "md_docs_2rendering-research-report.html#autotoc_md71", [
          [ "Option A: Single Unified Backend (Traditional)", "md_docs_2rendering-research-report.html#autotoc_md72", null ],
          [ "Option B: Hybrid Two-Tier (Recommended)", "md_docs_2rendering-research-report.html#autotoc_md73", null ]
        ] ]
      ] ],
      [ "3. Backend Technology Analysis", "md_docs_2rendering-research-report.html#autotoc_md75", [
        [ "3.1 Tier 1: UI Chrome Backends", "md_docs_2rendering-research-report.html#autotoc_md76", [
          [ "CPU Rasterizer (Reference Implementation)", "md_docs_2rendering-research-report.html#autotoc_md77", null ],
          [ "macOS: Core Graphics (CPU) or Metal (GPU)", "md_docs_2rendering-research-report.html#autotoc_md79", null ],
          [ "Windows: Direct2D (GPU) or GDI+ (CPU)", "md_docs_2rendering-research-report.html#autotoc_md81", null ],
          [ "Linux: Cairo (CPU/GPU) or Custom", "md_docs_2rendering-research-report.html#autotoc_md83", null ]
        ] ],
        [ "3.2 Tier 2: Viewport Backends", "md_docs_2rendering-research-report.html#autotoc_md85", [
          [ "macOS: Metal", "md_docs_2rendering-research-report.html#autotoc_md86", null ],
          [ "Linux: Vulkan", "md_docs_2rendering-research-report.html#autotoc_md88", null ],
          [ "Windows: DirectX 12 or Vulkan", "md_docs_2rendering-research-report.html#autotoc_md90", null ]
        ] ],
        [ "3.3 Cross-Platform Strategy", "md_docs_2rendering-research-report.html#autotoc_md92", null ]
      ] ],
      [ "4. Proposed Architecture", "md_docs_2rendering-research-report.html#autotoc_md94", [
        [ "4.1 Rendering Pipeline", "md_docs_2rendering-research-report.html#autotoc_md95", null ],
        [ "4.2 Key Abstractions", "md_docs_2rendering-research-report.html#autotoc_md96", [
          [ "Render Tree (New)", "md_docs_2rendering-research-report.html#autotoc_md97", null ],
          [ "UI Renderer Interface (Minimal)", "md_docs_2rendering-research-report.html#autotoc_md98", null ],
          [ "Viewport Interface (Direct Access)", "md_docs_2rendering-research-report.html#autotoc_md99", null ]
        ] ],
        [ "4.3 Frame Scheduling", "md_docs_2rendering-research-report.html#autotoc_md100", null ]
      ] ],
      [ "5. Implementation Phases", "md_docs_2rendering-research-report.html#autotoc_md102", [
        [ "Phase 4.1: Render Tree Construction", "md_docs_2rendering-research-report.html#autotoc_md103", null ],
        [ "Phase 4.2: Reference CPU Backend", "md_docs_2rendering-research-report.html#autotoc_md105", null ],
        [ "Phase 4.3: Platform Backend - macOS", "md_docs_2rendering-research-report.html#autotoc_md107", null ],
        [ "Phase 4.4: Platform Backend - Windows/Linux", "md_docs_2rendering-research-report.html#autotoc_md109", null ],
        [ "Phase 4.5: Viewport Integration", "md_docs_2rendering-research-report.html#autotoc_md111", null ],
        [ "Phase 4.6: Frame Scheduling & Vsync", "md_docs_2rendering-research-report.html#autotoc_md113", null ]
      ] ],
      [ "6. Visual Design Language", "md_docs_2rendering-research-report.html#autotoc_md116", [
        [ "6.1 Minimal Aesthetic (Neovim/Blender Style)", "md_docs_2rendering-research-report.html#autotoc_md117", null ],
        [ "6.2 Component Rendering", "md_docs_2rendering-research-report.html#autotoc_md118", null ]
      ] ],
      [ "7. Dependencies & Trade-offs", "md_docs_2rendering-research-report.html#autotoc_md120", [
        [ "7.1 Dependencies Matrix", "md_docs_2rendering-research-report.html#autotoc_md121", null ],
        [ "7.2 Trade-offs Analysis", "md_docs_2rendering-research-report.html#autotoc_md122", [
          [ "Option 1: Heavy Framework (Skia, Qt, etc.)", "md_docs_2rendering-research-report.html#autotoc_md123", null ],
          [ "Option 2: Pure CPU Rendering", "md_docs_2rendering-research-report.html#autotoc_md125", null ],
          [ "Option 3: Minimal Native Backends (Recommended)", "md_docs_2rendering-research-report.html#autotoc_md127", null ]
        ] ]
      ] ],
      [ "8. Success Criteria", "md_docs_2rendering-research-report.html#autotoc_md129", [
        [ "Phase 4 Acceptance", "md_docs_2rendering-research-report.html#autotoc_md130", null ]
      ] ],
      [ "9. Open Questions & Future Work", "md_docs_2rendering-research-report.html#autotoc_md132", [
        [ "Open Questions (Need Decisions)", "md_docs_2rendering-research-report.html#autotoc_md133", null ],
        [ "Future Work (Phase 5+)", "md_docs_2rendering-research-report.html#autotoc_md134", null ]
      ] ],
      [ "10. Recommendation", "md_docs_2rendering-research-report.html#autotoc_md136", [
        [ "Primary Recommendation: Hybrid Two-Tier Architecture", "md_docs_2rendering-research-report.html#autotoc_md137", null ],
        [ "Implementation Order", "md_docs_2rendering-research-report.html#autotoc_md138", null ],
        [ "Risk Mitigation", "md_docs_2rendering-research-report.html#autotoc_md139", null ]
      ] ],
      [ "11. Next Steps", "md_docs_2rendering-research-report.html#autotoc_md141", [
        [ "Immediate Actions", "md_docs_2rendering-research-report.html#autotoc_md142", null ],
        [ "First Pull Request", "md_docs_2rendering-research-report.html#autotoc_md143", null ]
      ] ],
      [ "Appendix A: Code Examples", "md_docs_2rendering-research-report.html#autotoc_md145", [
        [ "Example: Render Tree Builder", "md_docs_2rendering-research-report.html#autotoc_md146", null ],
        [ "Example: Platform Backend", "md_docs_2rendering-research-report.html#autotoc_md147", null ],
        [ "Example: Application Usage", "md_docs_2rendering-research-report.html#autotoc_md148", null ]
      ] ],
      [ "Appendix B: References", "md_docs_2rendering-research-report.html#autotoc_md150", [
        [ "Inspiration & Prior Art", "md_docs_2rendering-research-report.html#autotoc_md151", null ],
        [ "Technology References", "md_docs_2rendering-research-report.html#autotoc_md152", null ]
      ] ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Concepts", "concepts.html", "concepts" ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"classstdui_1_1platform__window.html#a85e49a24475341efaba60af101d74b5e",
"getting_started.html#autotoc_md40",
"roadmap.html",
"structstdui_1_1layout__node.html#ab88d166141164c2aa2bd57ecb0066be3"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';