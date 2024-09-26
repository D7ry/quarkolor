#include <iostream>

#include "ImGuiWidget.h"
#include "VulkanEngine.h"

void ImGuiWidgetEvenOdd::drawColorQuadTest()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    // ImGui::SetNextWindowFocus();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));
    ImGui::Begin("Color Quad Test");

    if (ImGui::Button("close")) {
        _drawQuadColorTest = false;
    }
    static int currentColor = 0;
    static int currentIntensity = 7; // Start at 128 (2^7)

    const char* colorNames[] = {"Red", "Green", "Blue", "Cyan", "Magenta", "Yellow"};
    ImGui::Combo("Primary Color", &currentColor, colorNames, IM_ARRAYSIZE(colorNames));

    ImGui::SliderInt("Intensity (Power of 2)", &currentIntensity, 0, 8);

    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImVec2 quadSize(windowSize.x, windowSize.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 quadMin = ImGui::GetCursorScreenPos();
    ImVec2 quadMax(quadMin.x + quadSize.x, quadMin.y + quadSize.y);

    // Calculate color based on selection
    int intensity = 1 << currentIntensity;
    ImU32 quadColor = 0;
    switch (currentColor) {
    case 0:
        quadColor = IM_COL32(intensity, 0, 0, 255);
        break; // Red
    case 1:
        quadColor = IM_COL32(0, intensity, 0, 255);
        break; // Green
    case 2:
        quadColor = IM_COL32(0, 0, intensity, 255);
        break; // Blue
    case 3:
        quadColor = IM_COL32(0, intensity, intensity, 255);
        break; // Cyan
    case 4:
        quadColor = IM_COL32(intensity, 0, intensity, 255);
        break; // Magenta
    case 5:
        quadColor = IM_COL32(intensity, intensity, 0, 255);
        break; // Yellow
    }

    drawList->AddRectFilled(quadMin, quadMax, quadColor);

    ImGui::PopStyleColor();
    ImGui::End();
}

void ImGuiWidgetEvenOdd::drawCalibrationWindow(VulkanEngine* engine, ColorSpace colorSpace)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    // ImGui::SetNextWindowFocus();
    //
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));

    ImGui::Begin(
        "Even Odd Test",
        NULL,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
    );

    if (ImGui::Button("close")) {
        _drawTestWindow = false;
    }

    if (colorSpace == ColorSpace::RGB) {
        ImGui::Text("Color Space: RGB");
    } else {
        ImGui::Text("Color Space: CMY");
    }

    ImGui::Text("Num Dropped Frame: %u", engine->_evenOddDebugCtx.numDroppedFrames);

    { // draw RGBCMY quads
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // RGBCMY
        ImU32 colors[6] = {
            IM_COL32(255, 0, 0, 255), // Red
            IM_COL32(0, 255, 0, 255), // Green
            IM_COL32(0, 0, 255, 255), // Blue
                                      //
            IM_COL32(255, 0, 0, 255), // Red
            IM_COL32(0, 255, 0, 255), // Green
            IM_COL32(0, 0, 255, 255), // Blue
        };

        // Get the window size
        ImVec2 windowSize = ImGui::GetWindowSize();

        // Calculate the width of each quad
        float quadWidth = windowSize.x / 6.0f;

        int iBegin;
        int iEnd;
        if (colorSpace == ColorSpace::RGB) {
            iBegin = 0;
            iEnd = 3;
        } else { // CMY
            iBegin = 3;
            iEnd = 6;
        }
        // Draw 6 evenly spaced quads
        for (int i = iBegin; i < iEnd; i++) {
            ImVec2 p0(i * quadWidth, windowSize.y * 0.4);
            ImVec2 p1((i + 1) * quadWidth, windowSize.y * 0.9);
            dl->AddRectFilled(p0, p1, colors[i]);
        }
    }

    ImGui::Checkbox("Flip RGB/CMY", &engine->_flipEvenOdd);
    if (engine->_tetraMode == VulkanEngine::TetraMode::kEvenOddSoftwareSync) {
        ImGui::SliderInt(
            "Software Sync Timing Offset (ns)",
            &engine->_softwareEvenOddCtx.timeOffset,
            0,
            engine->_softwareEvenOddCtx.nanoSecondsPerFrame
        );
        ImGui::Text(
            "Software Sync Frame Time (ns): %lu", engine->_softwareEvenOddCtx.nanoSecondsPerFrame
        );
    }

    if (!_calibrationInProgress) {
        if (ImGui::Button("Start Auto Calibration") && colorSpace == ColorSpace::RGB) {
            startAutoCalibration(engine);
        }
    }

    // Display calibration status
    if (_calibrationInProgress) {
        ImGui::Text("Calibration in progress...");
        ImGui::PushStyleColor(
            ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.7f, 0.3f, 1.0f)
        );                                                                       // Green progress
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f)); // Dark background
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::ProgressBar(_calibrationProgress, ImVec2{ImGui::GetWindowWidth() * 0.6f, 0});
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        if (ImGui::Button("Cancel Calibration")) {
            _calibrationInProgress = false;
        }
    }

    if (_calibrationComplete) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Bright green color
        ImGui::Text("Calibration complete.");
        ImGui::PopStyleColor();

        ImGui::Text("Optimal offset: ");
        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%d ns", _optimalOffset.load()
        ); // Light green color

        ImGui::Text("Highest dropped frames at worst offset: ");
        ImGui::SameLine();
        ImGui::TextColored(
            ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "%d", _highestDroppedFrames.load()
        ); // Slightly darker green
    }

    ImGui::PopStyleColor();
    ImGui::End();
}

void ImGuiWidgetEvenOdd::Draw(VulkanEngine* engine, ColorSpace colorSpace)
{
    const char* evenOddMode = nullptr;

    uint64_t numFrames = engine->getSurfaceCounterValue();
    switch (engine->_tetraMode) {
    case VulkanEngine::TetraMode::kEvenOddSoftwareSync: {
        evenOddMode = "Software Sync";
    } break;
    case VulkanEngine::TetraMode::kEvenOddHardwareSync:
        evenOddMode = "Hardware Sync";
        break;
    case VulkanEngine::TetraMode::kDualProjector:
        evenOddMode = "Dual Projector Does Not Use Even-Odd rendering";
        break;
    }
    ImGui::Text("Even odd mode: %s", evenOddMode);
    bool isEven = engine->isEvenFrame();

    ImGui::Text("Num Frame: %llu", engine->getSurfaceCounterValue());

    ImGui::Text("Num Dropped Frame: %u", engine->_evenOddDebugCtx.numDroppedFrames);

    if (engine->_tetraMode == VulkanEngine::TetraMode::kEvenOddSoftwareSync) {
        int buf = engine->_softwareEvenOddCtx.vsyncFrameOffset;
        if (ImGui::SliderInt("VSync frame offset", &buf, -10, 10)) {
            engine->_softwareEvenOddCtx.vsyncFrameOffset = buf;
        }
    }

    if (ImGui::Button("Draw Calibration Window")) {
        _drawTestWindow = true;
    }

    if (ImGui::Button("Draw Quad Color Test")) {
        _drawQuadColorTest = true;
    }

    if (_drawTestWindow) {
        drawCalibrationWindow(engine, colorSpace);
    }
    if (_drawQuadColorTest) {
        drawColorQuadTest();
    }

    ImGui::Text("Stress Test");

    if (_stressTesting) {
        ImGui::Text("Stress testing with %i threads...", NUM_STRESS_THREADS);
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            _stressTesting = false;
        }
    } else {
        if (ImGui::Button("Start")) {
            for (int i = 0; i < NUM_STRESS_THREADS; i++) {
                auto stressFunc = [this, i]() {
                    while (_stressTesting) {
                        std::cout << "here" << i << std::endl;
                    }
                    std::cout << i << "done" << std::endl;
                };
                _stressThreads[i] = std::thread(stressFunc);
                _stressThreads[i].detach();
            }
            _stressTesting = true;
        }
    }
}

int ImGuiWidgetEvenOdd::measureDroppedFrames(VulkanEngine* engine, int offset, int duration)
{
    engine->_softwareEvenOddCtx.timeOffset = offset;
    int initialDroppedFrames = engine->_evenOddDebugCtx.numDroppedFrames;

    std::this_thread::sleep_for(std::chrono::milliseconds(duration));

    return engine->_evenOddDebugCtx.numDroppedFrames - initialDroppedFrames;
}

void ImGuiWidgetEvenOdd::startAutoCalibration(VulkanEngine* engine)
{
    if (!_calibrationInProgress) {
        _calibrationInProgress = true;
        _calibrationProgress = 0.0f;
        _calibrationComplete = false;
        std::thread calibrationThread(&ImGuiWidgetEvenOdd::autoCalibrationThread, this, engine);
        calibrationThread.detach();
    }
}

void ImGuiWidgetEvenOdd::autoCalibrationThread(VulkanEngine* engine)
{
    int worstOffset = combinedCalibration(engine);
    int optimalOffset = (engine->_softwareEvenOddCtx.nanoSecondsPerFrame / 2 + worstOffset)
                        % engine->_softwareEvenOddCtx.nanoSecondsPerFrame;
    _optimalOffset = optimalOffset;
    if (_calibrationInProgress) {
        engine->_softwareEvenOddCtx.timeOffset = _optimalOffset;
        _calibrationComplete = true;
    }
    _calibrationInProgress = false;
}

int ImGuiWidgetEvenOdd::combinedCalibration(VulkanEngine* engine)
{
    int maxOffset = engine->_softwareEvenOddCtx.nanoSecondsPerFrame;
    int worstOffset = 0;
    _highestDroppedFrames = std::numeric_limits<int>::min();

    const float progBarPortionQuickScan = 0.8;
    const float progBarPortionRecursiveDescent = 1 - progBarPortionQuickScan;

    // Phase 1: Quick linear scan to find potential regions
    const int quickScanStep = maxOffset / 300;
    const int quickScanDuration = 30;
    std::vector<int> highFreqOffsets; // offsets observing high-frequency dropped frames

    for (int offset = 0; offset <= maxOffset; offset += quickScanStep) {
        int droppedFrames = measureDroppedFrames(engine, offset, quickScanDuration);
        if (droppedFrames > 0) {
            highFreqOffsets.push_back(offset);
        }
        _calibrationProgress = static_cast<float>(offset) / maxOffset * progBarPortionQuickScan;
        if (!_calibrationInProgress)
            return worstOffset;
    }

    // Phase 2: Recursive descent on potential regions
    const int minStepSize = maxOffset / 1000; // 0.1% of frame time
    float progressPerRegion = progBarPortionRecursiveDescent / highFreqOffsets.size();

    for (const auto& region : highFreqOffsets) {
        int start = std::max(0, region - quickScanStep);
        int end = std::min(maxOffset, region + quickScanStep);
        recursiveDescentCalibration(
            engine, start, end, minStepSize, worstOffset, progressPerRegion
        );
        if (!_calibrationInProgress)
            break;
    }

    return worstOffset;
}

void ImGuiWidgetEvenOdd::recursiveDescentCalibration(
    VulkanEngine* engine,
    int start,
    int end,
    int stepSize,
    int& worstOffset,
    float progressWeight // how much of progress does this descent take?
)
{
    const int recursiveDescentDuration = 150;
    if (end - start <= stepSize || !_calibrationInProgress)
        return;

    int mid = start + (end - start) / 2;
    int leftDropped = measureDroppedFrames(engine, start, recursiveDescentDuration);
    int midDropped = measureDroppedFrames(engine, mid, recursiveDescentDuration);
    int rightDropped = measureDroppedFrames(engine, end, recursiveDescentDuration);

    if (leftDropped > _highestDroppedFrames) {
        _highestDroppedFrames = leftDropped;
        worstOffset = start;
    }
    if (midDropped > _highestDroppedFrames) {
        _highestDroppedFrames = midDropped;
        worstOffset = mid;
    }
    if (rightDropped > _highestDroppedFrames) {
        _highestDroppedFrames = rightDropped;
        worstOffset = end;
    }

    // this complete, update prog wait -- recurse
    _calibrationProgress = _calibrationProgress + progressWeight / 3;

    if (leftDropped > midDropped || rightDropped > midDropped) {
        recursiveDescentCalibration(engine, start, mid, stepSize, worstOffset, progressWeight / 3);
        recursiveDescentCalibration(engine, mid, end, stepSize, worstOffset, progressWeight / 3);
    } else {
        _calibrationProgress = _calibrationProgress + progressWeight * 2 / 3;
    }
}
