#include "core/core.h"
#include "core/crash_handler.h"
#include "demo/demo.h"

int main() {
    CrashHandler::install();

    Core::init(Demo::init);
    Core::run(Demo::update);
    Core::shutdown(Demo::shutdown);

    return 0;
}