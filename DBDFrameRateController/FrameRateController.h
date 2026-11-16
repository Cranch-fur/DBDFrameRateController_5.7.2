#pragma once
#include "Utilities.h"






class FrameRateController
{
private:
	static inline const std::wstring customFrameRateCapCommandLineArgument = L"-frameRateCap=";
	static inline const std::wstring useMonitorRefreshRateCommandLineArgument = L"-useMonitorRefreshRate";
	static inline const std::wstring performanceStatsCommandLineArgument = L"-performanceStats";

	static inline float customFrameRateCap = 0.0f;
	static inline const float offlineFrameRateCap = 120.0f;
	static inline const float onlineFrameRateCap = 60.0f;

	static inline bool useMonitorRefreshRate = false;
	static inline float monitorRefreshRate = 0.0f;

	static inline bool showPerformanceStats = false;
	static inline bool performanceStatsShown = false;

	static void ApplyFrameRateCap();
	static DWORD WINAPI ThreadSafeWorker();


public:
	static void Init();
};