// NOTE(WALKER): This resume was built using imgui/examples/example_glfw_opengl3 folder as the basis.
//               The rest was modified from there to include my resume information as a software engineer.

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "../emscripten-browser-clipboard/emscripten_browser_clipboard.h"
#include "defer.hpp" // NOTE(WALKER): Custom defer macro used to make code sleaker and more readable when using imgui (especially begin()/end() pairs)

// NOTE(WALKER): This is a nice hack to get wrapped bullet text, not low level or deep, but it works well enough
#define IMGUI_BULLETTEXTWRAPPED(fmt_str, ...) ImGui::BulletText(""); ImGui::SameLine(); ImGui::TextWrapped(fmt_str, ##__VA_ARGS__)

void set_clipboard(void* data, const char* text) {
    emscripten_browser_clipboard::copy(text);
}

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../imgui/examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// NOTE(WALKER): This is the way ImGui does its font stuff so I put this here, not quite why this needs a namespace wrap though
namespace ImGui { IMGUI_API void ShowFontAtlas(ImFontAtlas* atlas); }

// Main code
int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    // GLFWwindow* window = glfwCreateWindow(3840, 2160, "Walker Williams Resume", nullptr, nullptr);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Walker Williams Resume", nullptr, nullptr);
    // GLFWwindow* window = glfwCreateWindow(1280, 720,  "Walker Williams Resume", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Set clipboard copy function:
    io.SetClipboardTextFn = set_clipboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // io.Fonts->AddFontDefault();
    // ImFontConfig config;
    // config.OversampleH = 2;
    // config.OversampleV = 1;
    // config.GlyphExtraSpacing.x = 1.0f;
    io.Fonts->AddFontFromFileTTF("fonts/JetBrainsMono-Regular.ttf", 26.0f);
    io.Fonts->AddFontFromFileTTF("fonts/LinLibertine_RBah.ttf", 26.0f);
    io.Fonts->AddFontFromFileTTF("fonts/times new roman.ttf", 26.0f);
    io.Fonts->AddFontFromFileTTF("fonts/ProggyClean.ttf", 26.0f);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGuiWindowFlags window_flags = 0;
            // window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_MenuBar;
            const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 0, main_viewport->WorkPos.y + 0), ImGuiCond_FirstUseEver);
            // ImGui::SetNextWindowSize(ImVec2(3840, 2160), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1920, 1080), ImGuiCond_FirstUseEver);
            // ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);

            ImGui::Begin("Walker Williams Resume", nullptr, window_flags);
            defer { ImGui::End(); };

            if (ImGui::BeginMenuBar()) {
                defer { ImGui::EndMenuBar(); };
                // Theme picker:
                if (ImGui::BeginMenu("Theme")) {
                    defer { ImGui::EndMenu(); };
                    static int theme_idx = 0;
                    if (ImGui::Combo(" ", &theme_idx, "Dark\0Light\0")) {
                        switch (theme_idx) {
                        case 0: ImGui::StyleColorsDark(); break;
                        case 1: ImGui::StyleColorsLight(); break;
                        }
                    }
                }
                // Font picker:
                if (ImGui::BeginMenu("Font")) {
                    defer { ImGui::EndMenu(); };
                    ImGui::ShowFontAtlas(io.Fonts);
                }
            }

            // Sections:
            if (ImGui::BeginTabBar("Sections")) {
                defer { ImGui::EndTabBar(); };
                if (ImGui::BeginTabItem("About", nullptr, ImGuiTabItemFlags_None)) {
                    defer { ImGui::EndTabItem(); };
                    ImGui::TextWrapped("Hello, my name is Walker Williams. I am a very passionate programmer and this is my resume, written in C++ and put on the web for you, recruiter, potential employer, or casual viewer, to look through. Thanks for stopping by, enjoy.");
                }
                // Skills:
                if (ImGui::BeginTabItem("Skills", nullptr, ImGuiTabItemFlags_None)) {
                    defer { ImGui::EndTabItem(); };
                    int open_action = -1;
                    if (ImGui::Button("Open all"))
                        open_action = 1;
                    ImGui::SameLine();
                    if (ImGui::Button("Close all"))
                        open_action = 0;
                    {
                        ImGui::BeginChild("Scroll");
                        defer { ImGui::EndChild(); };

                        // Languages:
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("C++ (master)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("C++ is my main language besides Jai");
                            IMGUI_BULLETTEXTWRAPPED("This resume is written in C++ thanks to the power of Dear ImGui and Emscripten");
                            IMGUI_BULLETTEXTWRAPPED("I have developed, from scratch solo, critical projects/architecture in C++, with no guidance");
                            IMGUI_BULLETTEXTWRAPPED("The above projects were all multi-threaded environments");
                            IMGUI_BULLETTEXTWRAPPED("I was well known at FlexGen Power Systems as one of their best C++ programmers, not even a linter was used without my approval");
                            IMGUI_BULLETTEXTWRAPPED("I know C++11, C++17 and C++20. C++20 is my favorite");
                            IMGUI_BULLETTEXTWRAPPED("Familiar with the STL, Generic Programming, creating custom allocators/memory management, etc.");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("golang (intermediate)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("I know golang enough to write code in it comfortably. I still have to look things up every now and then");                IMGUI_BULLETTEXTWRAPPED("I have mentored a new engineer to rewrite a core codebase from scratch in golang");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("python (intermediate)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("I know python at a scripting language level");
                            IMGUI_BULLETTEXTWRAPPED("I have written python scripts before in a professional environment");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Jai (intermediate)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Currently a member of the closed beta for Jai programming language");
                            IMGUI_BULLETTEXTWRAPPED("Beta access is not easily given out, you have to prove you're worthy of being given access");
                            IMGUI_BULLETTEXTWRAPPED("Only a couple hundred other people are in the beta");
                            IMGUI_BULLETTEXTWRAPPED("Beta is run by Jonathan Blow, creator of best selling games \"Braid\" and \"The Witness\"");

                            // TODO(WALKER): Figure out how to get a Jai link into the user's clipboard, almost there
                            // if (ImGui::Selectable(names[n], selected == n))
                            //     selected = n;
                            // if (ImGui::BeginPopupContextItem()) {
                            //     ImGui::Text("This a popup!");
                            //     if (ImGui::Button("Copy Link")) {
                            //         emscripten_browser_clipboard::copy("Hello copy world!");
                            //         ImGui::CloseCurrentPopup();
                            //     }
                            //     ImGui::EndPopup();
                            // }
                            // ImGui::SetItemTooltip("Right-click to copy Jai's community wiki link");
                        }
                        // Non-language stuff:
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Software Development Practices")) {
                            defer { ImGui::TreePop(); };
                            if (open_action != -1)
                                ImGui::SetNextItemOpen(open_action != 0);
                            if (ImGui::TreeNode("Agile Development")) {
                                defer { ImGui::TreePop(); };
                                IMGUI_BULLETTEXTWRAPPED("From filling out Jira tickets to inputing documentation in Markdown on Confluence or Github Wiki pages, I've done it all");
                                IMGUI_BULLETTEXTWRAPPED("Jira boards and filling out tickets");
                                IMGUI_BULLETTEXTWRAPPED("Markdown documentation on Confluence/Altissian");
                                IMGUI_BULLETTEXTWRAPPED("CI (Continuous Integration) using Github PRs (Pull Requests) and Jenkins/AWS");
                                IMGUI_BULLETTEXTWRAPPED("SCRUM meetings twice a week to check in on progress and potential blockers");
                                IMGUI_BULLETTEXTWRAPPED("Sprints that last 1 month+ with pre-planning");
                            }
                            if (open_action != -1)
                                ImGui::SetNextItemOpen(open_action != 0);
                            if (ImGui::TreeNode("Performance Aware Programming")) {
                                defer { ImGui::TreePop(); };
                                IMGUI_BULLETTEXTWRAPPED("I am NOT afraid of memory and low level programming");
                                IMGUI_BULLETTEXTWRAPPED("I am PRO creating custom allocators");
                                IMGUI_BULLETTEXTWRAPPED("I am PRO understanding how your program acesses memory and how that relates to CPU caches and RAM (L1, L2, L3, main memory cache misses and their costs");
                                IMGUI_BULLETTEXTWRAPPED("I am PRO SUA (Shutup Use Array), 99%% of the time. The last 1%% is usually a flat Hash Table");
                                IMGUI_BULLETTEXTWRAPPED("I am PRO writing software from scratch, from an empty main() and doing many iterations");
                                IMGUI_BULLETTEXTWRAPPED("These above princples I have used in professional environments to great success");
                            }
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Software Development Environment/Tools")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Docker for simulatating distributed systems and networks");
                            IMGUI_BULLETTEXTWRAPPED("Dockerfiles and docker-compose are nice tools");
                            IMGUI_BULLETTEXTWRAPPED("Linux environments are my goto, but Windows is alright");
                        }
                    }
                }
                // Work Experience:
                if (ImGui::BeginTabItem("Work Experience", nullptr, ImGuiTabItemFlags_None)) {
                    defer { ImGui::EndTabItem(); };
                    int open_action = -1;
                    if (ImGui::Button("Open all"))
                        open_action = 1;
                    ImGui::SameLine();
                    if (ImGui::Button("Close all"))
                        open_action = 0;
                    {
                        ImGui::BeginChild("Scroll");
                        defer { ImGui::EndChild(); };

                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Software Engineer - FlexGen Power Systems (2020 -> 2023)")) {
                            defer { ImGui::TreePop(); };

                            IMGUI_BULLETTEXTWRAPPED("Languages: C/C++11/17, golang, python, Bash");
                            IMGUI_BULLETTEXTWRAPPED("Joined FlexGen fresh out of college, my first job");
                            IMGUI_BULLETTEXTWRAPPED("FlexGen specializes in BESSs (Battery Energy Storage Systems), a form of distributed systems with lots of networking");
                            IMGUI_BULLETTEXTWRAPPED("I developed, along with 3 other people, FlexGen's ESS (Energy Storage System) Controller. This is one of their most important pieces of software and their lowest level controller");
                            IMGUI_BULLETTEXTWRAPPED("Because of the above point, FlexGen became a multi-million dollar success story, all of the software I wrote is still active today earning them millions more");
                            IMGUI_BULLETTEXTWRAPPED("I have rewritten core piece of code architecture for this company, from scratch, often solo and with the blessing of management");
                            IMGUI_BULLETTEXTWRAPPED("I have written their entire deployment/installation process originally in Bash. Later this was converted to Ansible by an entire team");
                            IMGUI_BULLETTEXTWRAPPED("I have made significant performance improvements across their whole system by rewriting the core IPC architecture that underlies everything. An end to end all possible input test went from 1 week down to 1 day or less");
                            IMGUI_BULLETTEXTWRAPPED("I have also rewritten core Modbus communication software that is used throughout the entire distributed network, bringing the CPU usage down from 112%% to 1-3%% in our largest use cases and increasing networking performance by about 2-3 times");
                            IMGUI_BULLETTEXTWRAPPED("Because of the above achievements FlexGen was able to properly scale to larger sites beyond 100+ MW, allowing them to take on some of the largest BESS projects in the world");
                            IMGUI_BULLETTEXTWRAPPED("I was known as one of their best Software Engineers, and could not be given the title of Senior Software Engineer only because I hadn't been there long enough (the lead of software at the time - John Calcagni - said so)");
                            IMGUI_BULLETTEXTWRAPPED("Because of the above achivements I have been given glowing recommendations on my LinkedIn profile from some of the software senior engineers and managers, everyone respected me and my expertise");
                        }
                    }
                }
                // Projects:
                if (ImGui::BeginTabItem("Projects", nullptr, ImGuiTabItemFlags_None)) {
                    defer { ImGui::EndTabItem(); };
                    int open_action = -1;
                    if (ImGui::Button("Open all"))
                        open_action = 1;
                    ImGui::SameLine();
                    if (ImGui::Button("Close all"))
                        open_action = 0;
                    {
                        ImGui::BeginChild("Scroll");
                        defer { ImGui::EndChild(); };

                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Hydroponics Startup - Currently Active")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Working with a previous FlexGen employee, Sam Rappl, on a hydroponics startup");
                            IMGUI_BULLETTEXTWRAPPED("Preliminary work is being done using Arduino Uno controllers, sensors, and custom circuits on a bread board");
                            IMGUI_BULLETTEXTWRAPPED("We have already applied for two government grants, each worth $100,000+");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Jai Language Closed Beta - Currently Active")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Currently a member of the closed beta for the Jai programming language");
                            IMGUI_BULLETTEXTWRAPPED("Beta access is not easily given out, you have to prove you're worthy of being given access");
                            IMGUI_BULLETTEXTWRAPPED("Only a couple hundred other people are in the beta");
                            IMGUI_BULLETTEXTWRAPPED("Beta is run by Jonathan Blow, creator of best selling games \"Braid\" and \"The Witness\"");
                            IMGUI_BULLETTEXTWRAPPED("I have filled out multiple bug reports across multiple beta versions already");
                            IMGUI_BULLETTEXTWRAPPED("I have contributed to an open source project that the beta members are writing called \"Focus\", an editor written 100%% in Jai that I am using right now to write this resume");
                        }
                    }
                }
                // Education:
                if (ImGui::BeginTabItem("Education", nullptr, ImGuiTabItemFlags_None)) {
                    defer { ImGui::EndTabItem(); };
                    int open_action = -1;
                    if (ImGui::Button("Open all"))
                        open_action = 1;
                    ImGui::SameLine();
                    if (ImGui::Button("Close all"))
                        open_action = 0;
                    {
                        ImGui::BeginChild("Scroll");
                        defer { ImGui::EndChild(); };

                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("University of North Carolina at Charlotte - (2016 -> 2020)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Early Master of Science in Computer Science - GPA: 3.8/4.0");
                            IMGUI_BULLETTEXTWRAPPED("Bachelor of Science in Computer Science     - GPA: 3.658/4.0");
                            IMGUI_BULLETTEXTWRAPPED("I enrolled in UNCC's Early Master's program");
                            IMGUI_BULLETTEXTWRAPPED("I graduated in 3 years with my Bachelor's and 4 with my Master's");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Lee Early College/Central Carolina Community College - (2012 -> 2016)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("High School Diploma && Associate in Science - GPA: 3.78/4.0");
                            IMGUI_BULLETTEXTWRAPPED("I was chosen out of Middle School for this High School through an interview process");
                            IMGUI_BULLETTEXTWRAPPED("Students would dual enroll in both High School and Community College courses, graduating from both at the end of 4 years");
                        }
                    }
                }
            }
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
