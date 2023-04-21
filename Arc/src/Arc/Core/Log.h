#pragma once

#include <spdlog/spdlog.h>

#include "LogFormatter.h"

namespace ArcEngine
{
	class Log
	{
	public:
		enum Level : uint32_t
		{
			Trace = 1,
			Debug = 2,
			Info = 4,
			Warn = 8,
			Error = 16,
			Critical = 32,
		};

	public:
		static void Init();
		
		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return  s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define ARC_CORE_TRACE(...)		::ArcEngine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ARC_CORE_INFO(...)		::ArcEngine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ARC_CORE_DEBUG(...)		::ArcEngine::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define ARC_CORE_WARN(...)		::ArcEngine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ARC_CORE_ERROR(...)		::ArcEngine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ARC_CORE_CRITICAL(...)	::ArcEngine::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define ARC_APP_TRACE(...)		::ArcEngine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ARC_APP_INFO(...)		::ArcEngine::Log::GetClientLogger()->info(__VA_ARGS__)
#define ARC_APP_DEBUG(...)		::ArcEngine::Log::GetClientLogger()->debug(__VA_ARGS__)
#define ARC_APP_WARN(...)		::ArcEngine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ARC_APP_ERROR(...)		::ArcEngine::Log::GetClientLogger()->error(__VA_ARGS__)
#define ARC_APP_CRITICAL(...)	::ArcEngine::Log::GetClientLogger()->critical(__VA_ARGS__)

#define ARC_APP_TRACE_EXTERNAL(file, line, function, ...)			::ArcEngine::Log::GetClientLogger()->log(spdlog::source_loc{ file, line, function }, spdlog::level::trace, __VA_ARGS__)
#define ARC_APP_INFO_EXTERNAL(file, line, function, ...)			::ArcEngine::Log::GetClientLogger()->log(spdlog::source_loc{ file, line, function }, spdlog::level::info, __VA_ARGS__)
#define ARC_APP_DEBUG_EXTERNAL(file, line, function, ...)			::ArcEngine::Log::GetClientLogger()->log(spdlog::source_loc{ file, line, function }, spdlog::level::debug, __VA_ARGS__)
#define ARC_APP_WARN_EXTERNAL(file, line, function, ...)			::ArcEngine::Log::GetClientLogger()->log(spdlog::source_loc{ file, line, function }, spdlog::level::warn, __VA_ARGS__)
#define ARC_APP_ERROR_EXTERNAL(file, line, function, ...)			::ArcEngine::Log::GetClientLogger()->log(spdlog::source_loc{ file, line, function }, spdlog::level::err, __VA_ARGS__)
#define ARC_APP_CRITICAL_EXTERNAL(file, line, function, ...)		::ArcEngine::Log::GetClientLogger()->log(spdlog::source_loc{ file, line, function }, spdlog::level::critical, __VA_ARGS__)
