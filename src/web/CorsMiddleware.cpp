// Created by Tube Lab. Part of the meloun project.
#include "web/CorsMiddleware.h"
using namespace ml::web;

CorsMiddleware::CorsMiddleware(const CorsRules& rules) noexcept
    : Rules_(rules)
{}

void CorsMiddleware::before_handle(crow::request& req, crow::response& res, context& ctx)
{
    res.add_header("Access-Control-Allow-Origin", Rules_.Origin.value_or(req.get_header_value("Origin")));
    res.add_header("Access-Control-Allow-Credentials", Rules_.AllowCredentials ? "true" : "false");
    res.add_header("Access-Control-Expose-Headers", Rules_.ExposeHeaders.value_or("*"));
    res.add_header("Access-Control-Max-Age", std::to_string(Rules_.MaxAge));
    res.add_header("Access-Control-Allow-Methods", MakeListOfMethods(Rules_.Methods));
    res.add_header("Access-Control-Allow-Headers", Rules_.Headers.value_or("*"));
}

void CorsMiddleware::after_handle(crow::request& req, crow::response& res, context& ctx)
{}

auto CorsMiddleware::MakeListOfMethods(const std::vector<crow::HTTPMethod>& methods) noexcept -> std::string
{
    std::string str;
    for (uint64_t i = 0; i < methods.size(); ++i)
    {
        str += method_name(methods[i]);
        str += (i == methods.size() - 1) ? "" : ", ";
    }

    return str;
}
