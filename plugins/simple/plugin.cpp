#include <sdfg/codegen/dispatchers/node_dispatcher_registry.h>
#include <sdfg/plugins/plugins.h>
#include <sdfg/serializer/json_serializer.h>
#include <list>
#include <memory>
#include <string>

#include "sdfg/structured_sdfg.h"
#include "simple_node.h"

extern "C" sdfg::plugins::Plugin register_docc_plugin() {
    return {
        .name = "simple",
        .version = "0.0.1",
        .description = "A simple example plugin",
        .register_plugin_callback = [](sdfg::plugins::Context& context) {},
        .sdfg_lookup = [](std::string name) {
            if (name == "_Z3foov") {
                return simple::create_foo();
            }
            return std::list<std::unique_ptr<sdfg::StructuredSDFG>>();
        }
    };
}
