// libbase/logging stub — upstream libbase/logging.cpp drags in Android
// liblog. We only need the few symbols that gui/LayerState.cpp + log/log.h
// macros reference from LOG/CHECK: LogMessage ctor/dtor/stream + ShouldLog.
// Everything prints (or fatals) to stderr.

#include <android-base/logging.h>

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace android::base {

// Opaque forward-declared upstream; give it a concrete body here.
class LogMessageData {
public:
  std::ostringstream os;
  const char *file;
  unsigned int line;
  LogSeverity severity;
  const char *tag;
  int error;
};

LogMessage::LogMessage(const char *file, unsigned int line, LogId /*log_id*/,
                       LogSeverity severity, const char *tag, int error)
    : data_(new LogMessageData{.file = file,
                               .line = line,
                               .severity = severity,
                               .tag = tag,
                               .error = error}) {}

LogMessage::LogMessage(const char *file, unsigned int line,
                       LogSeverity severity, const char *tag, int error)
    : data_(new LogMessageData{.file = file,
                               .line = line,
                               .severity = severity,
                               .tag = tag,
                               .error = error}) {}

LogMessage::~LogMessage() {
  static const char *const kLevels[] = {"V", "D", "I", "W", "E", "F", "F"};
  const auto sev = static_cast<int>(data_->severity);
  const char *level =
      (sev >= 0 && sev < (int)(sizeof(kLevels) / sizeof(kLevels[0])))
          ? kLevels[sev]
          : "?";
  std::cerr << "[" << level << " " << (data_->tag ? data_->tag : "") << "] "
            << data_->os.str() << std::endl;
  if (data_->severity == FATAL)
    std::abort();
}

std::ostream &LogMessage::stream() { return data_->os; }

void LogMessage::LogLine(const char * /*file*/, unsigned int /*line*/,
                         LogSeverity severity, const char *tag,
                         const char *msg) {
  std::cerr << "[" << (tag ? tag : "") << "] " << (msg ? msg : "") << std::endl;
  if (severity == FATAL)
    std::abort();
}

bool ShouldLog(LogSeverity /*severity*/, const char * /*tag*/) { return true; }

LogSeverity GetMinimumLogSeverity() { return VERBOSE; }
LogSeverity SetMinimumLogSeverity(LogSeverity /*new_severity*/) {
  return VERBOSE;
}

ScopedLogSeverity::ScopedLogSeverity(LogSeverity /*level*/) {}
ScopedLogSeverity::~ScopedLogSeverity() = default;

} // namespace android::base
