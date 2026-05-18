// Shared widget fixture for the scale harness.
//
// RenderWidgetZoo() emits a representative set of Dear ImGui widgets into the
// current frame. It is intentionally deterministic frame-to-frame (no animation,
// no time-based state) so PNG output from the headless harness is reproducible.
//
// This module is shared between the headless and interactive modes of the
// example_scale_harness binary, and will be reused in subsequent PRs as the
// visual fixture for PushView/PopView development.

#pragma once

namespace ScaleHarness {

// Emit the widget zoo into the current ImGui frame. Caller is responsible for
// having called NewFrame() and for calling Render() afterwards.
void RenderWidgetZoo();

} // namespace ScaleHarness
