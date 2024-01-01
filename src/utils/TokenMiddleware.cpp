// Created by Tube Lab. Part of the meloun project.
#include "utils/TokenMiddleware.h"
using namespace ml;

TokenMiddleware::TokenMiddleware(std::string_view token) noexcept
    : Token_(token)
{}

void TokenMiddleware::before_handle(crow::request& req, crow::response& res, TokenMiddleware::context& ctx)
{
    if (req.get_header_value("Authorization") != Token_)
    {
        res.code = 401;
        res.write("Unauthorized");
        res.end();
    }
}

void TokenMiddleware::after_handle(crow::request& req, crow::response& res, TokenMiddleware::context& ctx)
{}
