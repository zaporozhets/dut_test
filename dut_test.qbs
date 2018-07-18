/*
 * Copyright 2017 Taras Zaporozhets
 */

import qbs
Project {
    CppApplication {

        name: "dut_tests"

        consoleApplication: true
        cpp.warningLevel: "all"
        cpp.treatWarningsAsErrors: true
        cpp.cxxLanguageVersion: "c++11"


        files: [
            "src/**",
        ]
        cpp.includePaths: [
            "src/",
        ]

        cpp.libraryPaths: [

        ]

        cpp.dynamicLibraries: [
            "bluetooth"
        ]


        Group {
            fileTagsFilter: product.type
            qbs.install: true
            qbs.installDir: "/tmp/"
        }
    }
}

