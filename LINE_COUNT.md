# Codebase Line Count Report

**Repository:** TokyFy/hehe  
**Generated:** December 27, 2025

## Summary

- **Total Files:** 33 (excluding .git directory)
- **Total Lines:** 4,490

## Breakdown by File Type

| File Type | Lines | Percentage |
|-----------|-------|------------|
| C++ Source (.cpp) | 3,442 | 76.7% |
| C++ Headers (.hpp) | 588 | 13.1% |
| HTML | 362 | 8.1% |
| Other (Makefile, config) | 81 | 1.8% |
| Python | 17 | 0.4% |

## C++ Source Files (.cpp) - 3,442 lines

| File | Lines |
|------|-------|
| handleClients.cpp | 853 |
| config.cpp | 631 |
| Cgi.cpp | 489 |
| HttpServer.cpp | 394 |
| utils.cpp | 299 |
| HttpResponse.cpp | 250 |
| Client.cpp | 230 |
| HttpRequest.cpp | 203 |
| webserv.cpp | 58 |
| HttpAgent.cpp | 35 |

## C++ Header Files (.hpp) - 588 lines

| File | Lines |
|------|-------|
| HttpServer.hpp | 106 |
| Client.hpp | 117 |
| config.hpp | 85 |
| HttpResponse.hpp | 63 |
| HttpRequest.hpp | 58 |
| Cgi.hpp | 54 |
| handleClients.hpp | 39 |
| utils.hpp | 35 |
| HttpAgent.hpp | 31 |

## HTML Files - 362 lines

| File | Lines |
|------|-------|
| www/error_pages/400.html | 44 |
| www/error_pages/403.html | 44 |
| www/error_pages/404.html | 44 |
| www/error_pages/405.html | 44 |
| www/error_pages/413.html | 44 |
| www/error_pages/500.html | 44 |
| www/error_pages/504.html | 44 |
| www/public/upload.html | 30 |
| www/index.html | 24 |

## Other Files - 98 lines

| File | Lines |
|------|-------|
| www/conf/default.conf | 44 |
| Makefile | 30 |
| www/bin/test.py | 17 |
| .gitignore | 7 |

## Largest Files

1. **handleClients.cpp** - 853 lines
2. **config.cpp** - 631 lines
3. **Cgi.cpp** - 489 lines
4. **HttpServer.cpp** - 394 lines
5. **utils.cpp** - 299 lines

## Analysis

The codebase is primarily written in C++ (89.8% of total lines), implementing an HTTP web server. The code is well-modularized with separate components for:

- Client handling and connection management
- HTTP request/response parsing and generation
- CGI interface support
- Configuration parsing
- Server core functionality

The project includes error page templates and basic web content in the `www/` directory.
