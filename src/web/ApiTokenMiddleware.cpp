// Created by Tube Lab. Part of the meloun project.
#include "web/ApiTokenMiddleware.h"
using namespace ml::web;

ApiTokenMiddleware::ApiTokenMiddleware(std::string_view token) noexcept
    : Token_(token)
{}

void ApiTokenMiddleware::before_handle(crow::request& req, crow::response& res, context& ctx)
{
    if (req.get_header_value("Authorization") != Token_)
    {
        res.code = 401;
        res.write("401 Unauthorized");
        res.end();
    }
}

void ApiTokenMiddleware::after_handle(crow::request& req, crow::response& res, context& ctx)
{}
