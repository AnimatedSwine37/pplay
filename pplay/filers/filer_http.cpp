//
// Created by cpasjuste on 12/04/18.
//

#include <iostream>

#include "main.h"
#include "filer_http.h"
#include "Browser/Browser.hpp"
#include "utility.h"

using namespace c2d;

#define TIMEOUT 3

FilerHttp::FilerHttp(Main *main, const FloatRect &rect) : Filer(main, "", rect) {

    // http Browser
    browser = new Browser();
    browser->set_handle_gzip(true);
    browser->set_handle_redirect(true);
    browser->set_handle_ssl(false);
    browser->fetch_forms(false);
}

bool FilerHttp::getDir(const std::string &p) {

    printf("getDir(%s)\n", p.empty() ? browser->geturl().c_str() : p.c_str());

    if (!inited) {
        browser->open(p, TIMEOUT);
        inited = true;
    }

    if (browser->error() || browser->links.size() < 1) {
        printf("FilerHttp::getDir: %s\n", browser->getError().c_str());
        main->getStatus()->show("Error...", "Could not browse this folder...\n"
                                            "You should probably remove some special characters...");
        if (browser->get_history().size() > 1) {
            browser->back(TIMEOUT);
        }
        return false;
    }

    item_index = 0;
    files.clear();
    path = browser->unescape(browser->geturl());

    // add up/back ("..")
    files.emplace_back(Io::File("..", "..", Io::Type::Directory, 0, COLOR_BLUE_LIGHT), MediaInfo());

    for (int i = 0; i < browser->links.size(); i++) {

        // skip apache2 stuff
        if (browser->links[i].name() == ".."
            || browser->links[i].name() == "../"
            || browser->links[i].name() == "Name"
            || browser->links[i].name() == "Last modified"
            || browser->links[i].name() == "Size"
            || browser->links[i].name() == "Description"
            || browser->links[i].name() == "Parent Directory") {
            continue;
        }

        Io::Type type = Utility::endsWith(browser->links[i].name(), "/") ?
                        Io::Type::Directory : Io::Type::File;

        if (type == Io::Type::File) {
            std::string file_path = browser->geturl() + browser->escape(browser->links[i].name());
            Io::File file(browser->links[i].name(), file_path, type);
            if (pplay::Utility::isMedia(file)) {
                files.emplace_back(file, MediaInfo(file));
            }
        } else {
            Io::File file(Utility::removeLastSlash(browser->links[i].name()), browser->links[i].name(), type);
            files.emplace_back(file, MediaInfo(file));
        }
    }

    setSelection(0);

    return true;
}

void FilerHttp::enter(int prev_index) {

    MediaFile file = getSelection();

    if (file.name == "..") {
        exit();
        return;
    }

    browser->follow_link(file.path, TIMEOUT);
    if (getDir("")) {
        Filer::enter(prev_index);
    }
}

void FilerHttp::exit() {

    if (browser->get_history().size() > 1) {
        browser->back(TIMEOUT);
        if (getDir("")) {
            Filer::exit();
        }
    }
}

const std::string FilerHttp::getError() {
    return browser->getError();
}

FilerHttp::~FilerHttp() {
    delete (browser);
}
