#include <crow/common.h>
#include <optional>

namespace ml::web
{
    /** The CORS configuration. */
    struct CorsRules
    {
        /** Allowed origins, null-opt means that each origin will be allowed. */
        std::optional<std::string> Origin = std::nullopt;

        /** Allowed request headers, null-opt means that each header will be allowed.  */
        std::optional<std::string> Headers = std::nullopt;

        /** Determines the list of allowed request methods. */
        std::vector<crow::HTTPMethod> Methods = { "GET"_method, "POST"_method };

        /** Whether requests with credentials will be allowed. */
        bool AllowCredentials = true;

        /** Maximal lifetime of the request ( 600 due to google chrome limitations ). */
        uint MaxAge = 600;

        /** Allowed response headers, null-opt means that each header will be allowed. */
        std::optional<std::string> ExposeHeaders = "";
    };
}