#pragma once
#include <iostream>
#include <stdio.h>
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "examples/libs/emscripten/emscripten_mainloop_stub.h"
#include<string>
#include<fstream>
#include <iomanip>
#include "Xertz.h"
#include <Windows.h>
#include "MungPlexConfig.h"
#include <sstream>
#include"ProcessInformation.h"
#include"HelperFunctions.h"

namespace MungPlex
{
    class Connection
    {
    private:
        Connection()
        {

        }

        ~Connection() {};
        Connection(const Connection&) = delete;
        Connection(Connection&&) = delete;
        void operator=(const Connection&) = delete;
        void operator=(Connection&&) = delete;
        static Connection& GetInstance()
        {
            static Connection Instance;
            return Instance;
        }

        bool _connected = false;
        void DrawConnectionSelect();
        void ParseJsonToEntities();
        int _selectedEmulatorIndex = 0;

    public:
        static void DrawWindow();
        static bool IsConnected();
    };
}