// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "CorsRules.h"
#include <crow/middleware.h>

namespace ml::web
{
    /**
     * @brief Basic middleware for managing CORS policies.
     * @safety Fully exception and thread safe.
     *
     * Warnings:
     * - Will not work with preflight requests ( due to crow limitations ).
     */
    class CorsMiddleware
    {
        CorsRules Rules_;

    public:
        struct context {};

        /** Creates the middleware, the config can't be changed later. */
        CorsMiddleware(const CorsRules& rules) noexcept;

        /** Validates CORS policies in .  */
        void before_handle(crow::request& req, crow::response& res, context& ctx);

        /** Does nothing. */
        void after_handle(crow::request& req, crow::response& res, context& ctx);

    private:
        static auto MakeListOfMethods(const std::vector<crow::HTTPMethod>& methods) noexcept -> std::string;
    };
}