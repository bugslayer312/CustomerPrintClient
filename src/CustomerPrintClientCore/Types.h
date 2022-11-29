#pragma once

#include <memory>
#include <list>
#include <functional>

struct PrintOffice;
struct PrintJobInfo;

typedef std::shared_ptr<PrintOffice> PrintOfficePtr;
typedef std::shared_ptr<PrintJobInfo> PrintJobInfoPtr;
typedef std::list<PrintJobInfoPtr> PrintJobInfoList;
typedef std::shared_ptr<PrintJobInfoList> PrintJobInfoListPtr;

typedef std::function<void()> VoidCallback;
typedef std::function<void(bool)> BoolCallback;