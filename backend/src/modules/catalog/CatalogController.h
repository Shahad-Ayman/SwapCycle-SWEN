#pragma once

// HTTP endpoints for the catalog module: /api/books/*, /api/users/me/books, /api/photos/*.

#include <drogon/HttpController.h>

#include <memory>
#include <string>

#include "CatalogService.h"

#ifndef SWAPCYCLE_UUID_RE
#define SWAPCYCLE_UUID_RE "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"
#endif

namespace swapcycle::catalog {

class CatalogController final : public drogon::HttpController<CatalogController, false> {
public:
    explicit CatalogController(std::shared_ptr<CatalogService> service);

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CatalogController::create, "/api/books", drogon::Post, "AuthFilter");
    ADD_METHOD_TO(CatalogController::listMine, "/api/users/me/books", drogon::Get, "AuthFilter");
    ADD_METHOD_VIA_REGEX(CatalogController::get, "/api/books/(" SWAPCYCLE_UUID_RE ")", drogon::Get);
    ADD_METHOD_VIA_REGEX(CatalogController::update, "/api/books/(" SWAPCYCLE_UUID_RE ")", drogon::Patch,
                         "AuthFilter");
    ADD_METHOD_VIA_REGEX(CatalogController::remove, "/api/books/(" SWAPCYCLE_UUID_RE ")", drogon::Delete,
                         "AuthFilter");
    ADD_METHOD_VIA_REGEX(CatalogController::withdraw, "/api/books/(" SWAPCYCLE_UUID_RE ")/withdraw", drogon::Post,
                         "AuthFilter");
    ADD_METHOD_VIA_REGEX(CatalogController::relist, "/api/books/(" SWAPCYCLE_UUID_RE ")/relist", drogon::Post,
                         "AuthFilter");
    ADD_METHOD_VIA_REGEX(CatalogController::addPhoto, "/api/books/(" SWAPCYCLE_UUID_RE ")/photos", drogon::Post,
                         "AuthFilter");
    ADD_METHOD_VIA_REGEX(CatalogController::deletePhoto,
                         "/api/books/(" SWAPCYCLE_UUID_RE ")/photos/(" SWAPCYCLE_UUID_RE ")", drogon::Delete,
                         "AuthFilter");
    ADD_METHOD_VIA_REGEX(CatalogController::photo, "/api/photos/(" SWAPCYCLE_UUID_RE ")", drogon::Get);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> listMine(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> get(drogon::HttpRequestPtr req, std::string copyId);
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req, std::string copyId);
    drogon::Task<drogon::HttpResponsePtr> remove(drogon::HttpRequestPtr req, std::string copyId);
    drogon::Task<drogon::HttpResponsePtr> withdraw(drogon::HttpRequestPtr req, std::string copyId);
    drogon::Task<drogon::HttpResponsePtr> relist(drogon::HttpRequestPtr req, std::string copyId);
    drogon::Task<drogon::HttpResponsePtr> addPhoto(drogon::HttpRequestPtr req, std::string copyId);
    drogon::Task<drogon::HttpResponsePtr> deletePhoto(drogon::HttpRequestPtr req, std::string copyId,
                                                      std::string photoId);
    drogon::Task<drogon::HttpResponsePtr> photo(drogon::HttpRequestPtr req, std::string photoId);

private:
    std::shared_ptr<CatalogService> service_;
};

}  // namespace swapcycle::catalog
