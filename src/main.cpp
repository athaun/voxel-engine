#include "core/core.h"
#include "demo/demo.h"

int main() {
    Core::init(Demo::init);
    Core::run(Demo::update);
    Core::shutdown(Demo::shutdown);

    return 0;
}