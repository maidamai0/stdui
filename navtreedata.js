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
    [ "Phase 4: Rendering Subsystem Design", "md_docs_2phase4-rendering.html", [
      [ "Executive Summary", "md_docs_2phase4-rendering.html#autotoc_md60", [
        [ "Core Architecture", "md_docs_2phase4-rendering.html#autotoc_md61", null ]
      ] ],
      [ "1. Architecture Overview", "md_docs_2phase4-rendering.html#autotoc_md63", [
        [ "1.1 The Complete Picture", "md_docs_2phase4-rendering.html#autotoc_md64", null ],
        [ "1.2 Why GPU-Based UI?", "md_docs_2phase4-rendering.html#autotoc_md65", null ]
      ] ],
      [ "2. Platform Rendering Backends", "md_docs_2phase4-rendering.html#autotoc_md67", [
        [ "2.1 Technology Selection", "md_docs_2phase4-rendering.html#autotoc_md68", null ],
        [ "2.2 macOS: Metal + Core Text", "md_docs_2phase4-rendering.html#autotoc_md69", null ],
        [ "2.3 Windows: Direct2D + DirectWrite", "md_docs_2phase4-rendering.html#autotoc_md70", null ],
        [ "2.4 Linux: Skia + Vulkan + HarfBuzz", "md_docs_2phase4-rendering.html#autotoc_md71", null ]
      ] ],
      [ "3. Text Rendering Deep Dive", "md_docs_2phase4-rendering.html#autotoc_md73", [
        [ "3.1 Dynamic Glyph Atlas System", "md_docs_2phase4-rendering.html#autotoc_md74", null ],
        [ "3.2 The Three-Stage Process", "md_docs_2phase4-rendering.html#autotoc_md75", null ],
        [ "3.3 Glyph Atlas Implementation", "md_docs_2phase4-rendering.html#autotoc_md76", null ]
      ] ],
      [ "4. Threading & Synchronization", "md_docs_2phase4-rendering.html#autotoc_md78", [
        [ "4.1 Two-Thread Architecture", "md_docs_2phase4-rendering.html#autotoc_md79", null ],
        [ "4.2 GPU Command Queues", "md_docs_2phase4-rendering.html#autotoc_md80", null ],
        [ "4.3 OS Compositor Integration", "md_docs_2phase4-rendering.html#autotoc_md81", null ]
      ] ],
      [ "5. Effects Implementation", "md_docs_2phase4-rendering.html#autotoc_md83", [
        [ "5.1 Offscreen Rendering", "md_docs_2phase4-rendering.html#autotoc_md84", null ],
        [ "5.2 Dual-Kawase Blur (Production Quality)", "md_docs_2phase4-rendering.html#autotoc_md85", null ],
        [ "5.3 Drop Shadow", "md_docs_2phase4-rendering.html#autotoc_md86", null ],
        [ "5.4 Rounded Corners (SDF Method)", "md_docs_2phase4-rendering.html#autotoc_md87", null ]
      ] ],
      [ "6. Public API Design", "md_docs_2phase4-rendering.html#autotoc_md89", [
        [ "6.1 Application Entry Point", "md_docs_2phase4-rendering.html#autotoc_md90", null ],
        [ "6.2 Scene View Integration", "md_docs_2phase4-rendering.html#autotoc_md91", null ]
      ] ],
      [ "Core library (header-only)", "md_docs_2phase4-rendering.html#autotoc_md92", null ],
      [ "Platform-specific rendering (dispatched to separate files)", "md_docs_2phase4-rendering.html#autotoc_md93", null ],
      [ "Testing, docs, install", "md_docs_2phase4-rendering.html#autotoc_md94", null ],
      [ "Rendering implementation library", "md_docs_2phase4-rendering.html#autotoc_md95", null ],
      [ "Platform-specific configuration (dispatched)", "md_docs_2phase4-rendering.html#autotoc_md96", null ],
      [ "Link rendering to main target", "md_docs_2phase4-rendering.html#autotoc_md97", null ],
      [ "Done! Backend selected automatically by platform", "md_docs_2phase4-rendering.html#autotoc_md98", [
        [ "8. Implementation Plan", "md_docs_2phase4-rendering.html#autotoc_md100", [
          [ "Phase 4.1: Render Tree Construction (2 weeks)", "md_docs_2phase4-rendering.html#autotoc_md101", null ],
          [ "Phase 4.2: Metal Backend - macOS (3 weeks)", "md_docs_2phase4-rendering.html#autotoc_md102", null ],
          [ "Phase 4.3: Direct2D Backend - Windows (3 weeks)", "md_docs_2phase4-rendering.html#autotoc_md103", null ],
          [ "Phase 4.4: Skia/Vulkan Backend - Linux (3 weeks)", "md_docs_2phase4-rendering.html#autotoc_md104", null ],
          [ "Phase 4.5: Scene View Integration (2 weeks)", "md_docs_2phase4-rendering.html#autotoc_md105", null ],
          [ "Phase 4.6: Polish & Performance (2 weeks)", "md_docs_2phase4-rendering.html#autotoc_md106", null ]
        ] ],
        [ "9. Success Criteria", "md_docs_2phase4-rendering.html#autotoc_md108", [
          [ "Functional Requirements", "md_docs_2phase4-rendering.html#autotoc_md109", null ],
          [ "Quality Requirements", "md_docs_2phase4-rendering.html#autotoc_md110", null ],
          [ "Code Quality", "md_docs_2phase4-rendering.html#autotoc_md111", null ]
        ] ],
        [ "10. Summary", "md_docs_2phase4-rendering.html#autotoc_md113", [
          [ "Key Architectural Decisions", "md_docs_2phase4-rendering.html#autotoc_md114", null ],
          [ "What's Different from Other Frameworks", "md_docs_2phase4-rendering.html#autotoc_md115", null ],
          [ "Next Phase (Phase 5)", "md_docs_2phase4-rendering.html#autotoc_md116", null ]
        ] ]
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
"structstdui_1_1app__config.html",
"structstdui_1_1mouse__event.html#a7c747ac5b9641d3f26e45c07dc845324"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';