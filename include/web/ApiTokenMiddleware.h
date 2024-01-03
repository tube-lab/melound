// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <crow/middleware.h>

namespace ml::web
{
    /**
     * @brief Basic middleware for permitting access to API by constant token.
     * @safety Fully exception and thread safe.
     */
    class ApiTokenMiddleware
    {
        std::string Token_;

    public:
        struct context {};

        /** Creates the middleware, the token can't be changed later. */
        ApiTokenMiddleware(std::string_view token) noexcept;

        /** Checks whether the valid access token is provided within the request. */
        void before_handle(crow::request& req, crow::response& res, context& ctx);

        /** Does nothing. */
        void after_handle(crow::request& req, crow::response& res, context& ctx);
    };
}