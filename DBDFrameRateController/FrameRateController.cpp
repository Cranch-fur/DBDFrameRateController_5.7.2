#include "FrameRateController.h"






void FrameRateController::ApplyFrameRateCap()
{
	/* Dead by Daylight 5.7.2 */
	static const size_t SCAN_POS = 0x0B6F6CE0;

	HMODULE hMainModule = GetModuleHandle(nullptr);
	uintptr_t tMaxFPSAddress = Utilities::Memory::Internal::AddressDereference(reinterpret_cast<uintptr_t>(hMainModule), SCAN_POS);

	if (tMaxFPSAddress == 0x0)
		return;

	float frameRateCap = 0.0f;
	if (customFrameRateCap > 0.0f)
		frameRateCap = customFrameRateCap;
	else
	{
		/*
			if (Utilities::Game::IsInOnlineMatch())
				frameRateCap = onlineFrameRateCap;
			else
		*/
		frameRateCap = useMonitorRefreshRate ? monitorRefreshRate : offlineFrameRateCap;
	}

	Utilities::Memory::Internal::SetFloat(tMaxFPSAddress, frameRateCap);

	if (showPerformanceStats && (performanceStatsShown == false))
	{
		SDK::UWorld* world = SDK::UWorld::GetWorld();
		if (world == nullptr)
			return;

		if (Utilities::Game::IsAtStartScreen(world) == false)
			return;

		SDK::UKismetSystemLibrary::ExecuteConsoleCommand(world, L"stat FPS", nullptr);
		SDK::UKismetSystemLibrary::ExecuteConsoleCommand(world, L"stat Unit", nullptr);
		SDK::UKismetSystemLibrary::ExecuteConsoleCommand(world, L"stat UnitGraph", nullptr);
		SDK::UKismetSystemLibrary::ExecuteConsoleCommand(world, L"stat UnitMax", nullptr);
		SDK::UKismetSystemLibrary::ExecuteConsoleCommand(world, L"stat Hitches", nullptr);
		SDK::UKismetSystemLibrary::ExecuteConsoleCommand(world, L"stat AI", nullptr);
		SDK::UKismetSystemLibrary::ExecuteConsoleCommand(world, L"stat Summary", nullptr);
		performanceStatsShown = true;
	}
}

DWORD __stdcall FrameRateController::ThreadSafeWorker()
{
	printf("[DBDFrameRateController] ThreadSafeWorker() is now entering the loop\n");

	while (true)
	{
		EXCEPTION_POINTERS* exceptionInfo = nullptr;

		__try
		{
			ApplyFrameRateCap();
			Sleep(100);
		}
		__except (exceptionInfo = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER)
		{
			if (exceptionInfo)
			{
				void* crashAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
				DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;

				HMODULE hModule = GetModuleHandle(nullptr);
				uintptr_t baseAddress = (uintptr_t)hModule;
				uintptr_t relativeOffset = (uintptr_t)crashAddress - baseAddress;

				printf("[DBDFrameRateController] FATAL ERROR / CRASH in %s\n", __FUNCSIG__);
				printf("[DBDFrameRateController] Exception Code:   0x%08X\n", exceptionCode);
				printf("[DBDFrameRateController] Absolute Address: 0x%llX\n", (unsigned long long)crashAddress);
				printf("[DBDFrameRateController] Module Base:      0x%llX\n", (unsigned long long)baseAddress);
				printf("[DBDFrameRateController] Relative Offset:  0x%llX\n", (unsigned long long)relativeOffset);
			}

			printf("[DBDFrameRateController] Restarting loop in 3 seconds...\n");
			Sleep(3000);
		}
	}

	return 0;
}




void FrameRateController::Init()
{
	printf("[DBDFrameRateController] Starting initialization sequence...\n");
	while (Utilities::Game::IsInitialized() == false)
	{
		Sleep(10);
	}

	printf("[DBDFrameRateController] Game is initialized, looking up command line...\n");
	std::vector<std::wstring> processArguments = Utilities::CommandLine::GetArgumentsUtf16();
	for (std::wstring& arg : processArguments)
	{
		if (Utilities::String::StartsWith(arg, customFrameRateCapCommandLineArgument))
		{
			wprintf(L"[DBDFrameRateController] Found command line argument \"%ls\"\n", arg.c_str());

			std::vector<std::wstring> split = Utilities::String::Split(arg, L'=', false);
			if (split.size() >= 2)
			{
				wchar_t* endPtr = nullptr;
				float frameRateCap = std::wcstof(split[1].c_str(), &endPtr);

				if (endPtr != split[1].c_str() && frameRateCap > 0.0f)
				{
					customFrameRateCap = frameRateCap;
					printf("[DBDFrameRateController] customFrameRateCap = %.2f\n", customFrameRateCap);
				}
				else
				{
					printf("[DBDFrameRateController] Invalid command line argument float value or value is <= 0\n");
				}
			}
			else
				printf("[DBDFrameRateController] Incorrect command line argument\n");
		}
		else if (Utilities::String::StartsWith(arg, useMonitorRefreshRateCommandLineArgument))
		{
			wprintf(L"[DBDFrameRateController] Found command line argument \"%ls\"\n", arg.c_str());
			printf("[DBDFrameRateController] useMonitorRefreshRate = true\n");
			useMonitorRefreshRate = true;
		}
		else if (Utilities::String::StartsWith(arg, performanceStatsCommandLineArgument))
		{
			wprintf(L"[DBDFrameRateController] Found command line argument \"%ls\"\n", arg.c_str());
			printf("[DBDFrameRateController] showPerformanceStats = true\n");
			showPerformanceStats = true;
		}
	}

	if (useMonitorRefreshRate)
	{
		printf("[DBDFrameRateController] Detecting monitor refresh rate...\n");
		DEVMODE dm = {};
		dm.dmSize = sizeof(dm);

		if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm))
		{
			uint32_t displayFrequency = dm.dmDisplayFrequency;

			static const uint32_t commonRefreshRates[] = { 30, 48, 50, 60, 72, 75, 85, 90, 100, 120, 144, 165, 175, 180, 200, 240, 280, 360, 480, 500, 540, 600 };
			static const uint32_t diffTolerance = 1;
			bool refreshRateTolerated = false;

			for (uint32_t commRfRt : commonRefreshRates)
			{
				if (displayFrequency >= (commRfRt - diffTolerance) && displayFrequency <= (commRfRt + diffTolerance))
				{
					monitorRefreshRate = static_cast<float>(commRfRt);
					refreshRateTolerated = true;
					printf("[DBDFrameRateController] Refresh rate tolerated: %u Hz -> %.0f Hz\n", displayFrequency, monitorRefreshRate);
					break;
				}
			}

			if (refreshRateTolerated == false)
			{
				if (displayFrequency >= 30 && displayFrequency <= 999)
				{
					monitorRefreshRate = static_cast<float>(displayFrequency);
					printf("[DBDFrameRateController] Using raw monitor refresh rate: %.0f Hz\n", monitorRefreshRate);
				}
				else
				{
					monitorRefreshRate = 60.0f;
					printf("[DBDFrameRateController] Invalid reported refresh rate (%u), falling back to 60.0f Hz\n", displayFrequency);
				}
			}
		}
		else
		{
			monitorRefreshRate = 60.0f;
			printf("[DBDFrameRateController] EnumDisplaySettings failed, falling back to 60.0f Hz\n");
		}
	}

	printf("[DBDFrameRateController] Starting ThreadSafeWorker()...\n");
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadSafeWorker, 0, 0, 0);
}