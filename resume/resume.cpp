// NOTE(WALKER): This resume was built using imgui/examples/example_glfw_opengl3 folder as the basis.
//               The rest was modified from there to include my resume information as a software engineer.

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_freetype.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "../emscripten-browser-clipboard/emscripten_browser_clipboard.h"
#include "../Utilities/defer.hpp" // NOTE(WALKER): Custom defer macro used to make code sleaker and more readable when using imgui (especially begin()/end() pairs)

// NOTE(WALKER): This is the text from defer.hpp as a const char[] so people can view it:
static const char defer_macro_code[] = R"DELIMITER(
#ifndef DEFER_CPP
#define DEFER_CPP

// A useful macro to get arbitrary code to execute at the end of a scope using RAII and lambdas.
// This provides defer syntax similiar to golang. This is useful for interfacing
// with C APIs that need a "deinit" or "cleanup" function to be called at the end of scope.
// This creates code that is cleaner because you don't need to create lots of RAII wrappers.

// NOTE: The defer macro is assumed to be "noexcept" by default.
//       #define DEFER_WITH_EXCEPTIONS above "defer.hpp" to allow exceptions to be thrown
//       inside defer statements (although I would highly advise against it)
#ifdef DEFER_WITH_EXCEPTIONS
#define DEFER_NOEXCEPT noexcept(false)
#else
#define DEFER_NOEXCEPT noexcept(true)
#endif

#if __cplusplus >= 201703L

// Defer macro (>= C++17):
template<typename Code>
struct Defer {
    Code code;
// constexpr support (>= C++20):
#if __cplusplus >= 202002L
    constexpr Defer(Code block) DEFER_NOEXCEPT : code(block) {}
    constexpr ~Defer() DEFER_NOEXCEPT { code(); }
#else
    Defer(Code block) DEFER_NOEXCEPT : code(block) {}
    ~Defer() DEFER_NOEXCEPT { code(); }
#endif
};
#define GEN_DEFER_NAME_HACK(name, counter) name ## counter
#define GEN_DEFER_NAME(name, counter) GEN_DEFER_NAME_HACK(name, counter)
#define defer Defer GEN_DEFER_NAME(_defer_, __COUNTER__) = [&]() DEFER_NOEXCEPT

#else

// Defer macro (>= C++11)
template<typename Code>
struct Defer {
    Code code;
    Defer(Code block) DEFER_NOEXCEPT : code(block) {}
    ~Defer() DEFER_NOEXCEPT { code(); }
};
struct Defer_Generator { template<typename Code> Defer<Code> operator +(Code code) DEFER_NOEXCEPT { return Defer<Code>{code}; } };
#define GEN_DEFER_NAME_HACK(name, counter) name ## counter
#define GEN_DEFER_NAME(name, counter) GEN_DEFER_NAME_HACK(name, counter)
#define defer auto GEN_DEFER_NAME(_defer_, __COUNTER__) = Defer_Generator{} + [&]() DEFER_NOEXCEPT

#endif

// Example usage:
// auto some_func(auto& input) {
//     defer { ++input; /* Put code block here to execute at end of scope, you can refer to "input" in this code block like normal */ };
//     // put other code here you want to execute before defer like normal
//     return input;
// }
// Example main (should print 1 before 2):
// int main() {
//     defer { printf("hello, defer world! 2\n"); };
//     defer { printf("hello, defer world! 1\n"); };
// }

#endif
)DELIMITER";

// NOTE(WALKER): This is a nice hack to get wrapped bullet text, not low level or deep, but it works well enough
#define IMGUI_BULLETTEXTWRAPPED(fmt_str, ...) ImGui::BulletText(""); ImGui::SameLine(); ImGui::TextWrapped(fmt_str, ##__VA_ARGS__)

void set_clipboard(void* data, const char* text) {
    emscripten_browser_clipboard::copy(text);
}

// NOTE(WALKER): For getting the canvas width/height in order to make a window
EM_JS(int, get_canvas_width, (), {
    return Module.canvas.width;
});
EM_JS(int, get_canvas_height, (), {
    return Module.canvas.height;
});
// NOTE(WALKER): Called at the beginning to set canvas dimensions to the dimensions of the user's monitor
// credit: https://stackoverflow.com/questions/16277383/javascript-screen-height-and-screen-width-returns-incorrect-values
EM_JS(void, resize_canvas_to_screen_dimensions, (), {
    var w = screen.width; var h = screen.height;
    var DPR = window.devicePixelRatio;
    w = Math.round(DPR * w);
    h = Math.round(DPR * h);
    if (w < h) {
        var temp = w;
        w = h;
        h = temp;
    }
    Module.canvas.width  = w;
    Module.canvas.height = h;
});
// NOTE(WALKER): This is so we can click on links we want to embed into the UI (it uses the clipboard that we copy into)
EM_JS(void, open_link_through_clipboard, (), {
    navigator.clipboard.readText().then(
        (clipText) => (window.open(clipText, "_blank"))
    );
});

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

    // Automatically set Canvas Width/Height:
    resize_canvas_to_screen_dimensions();
    auto canvas_width  = get_canvas_width();
    auto canvas_height = get_canvas_height();

    // NOTE(WALKER): This is for mobile users so they can at least view the app when they turn their phone sideways
    //               They still wno't be able to interact with it very well because touch screens are wonky
    if (canvas_width < canvas_height) {
        auto temp = canvas_width;
        canvas_width = canvas_height;
        canvas_height = temp;
    }

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(canvas_width, canvas_height, "Walker Williams Resume", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Set clipboard copy function:
    io.SetClipboardTextFn = set_clipboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    // Rounding:
    style.WindowRounding = 0.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 12.0f;

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // NOTE(WALKER): Automatically adjust font size based on the user's window dimensions to get clarity
    const auto font_ratio = (canvas_width / 960.0f);
    io.Fonts->AddFontFromFileTTF("fonts/JetBrainsMono-Regular.ttf", 13.0f * font_ratio);
    io.Fonts->AddFontFromFileTTF("fonts/LiberationSans-Regular.ttf", 13.0f * font_ratio);
    io.Fonts->AddFontFromFileTTF("fonts/LinLibertine_RBah.ttf", 13.0f * font_ratio);
    io.Fonts->AddFontFromFileTTF("fonts/times new roman.ttf", 13.0f * font_ratio);
    io.Fonts->AddFontFromFileTTF("fonts/ProggyClean.ttf", 13.0f * font_ratio);

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
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);

            ImGuiWindowFlags window_flags = 0;
            // window_flags |= ImGuiWindowFlags_NoTitleBar;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoResize;
            window_flags |= ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_MenuBar;
            ImGui::Begin("Walker Williams Resume (Written in C++)", nullptr, window_flags);
            defer { ImGui::End(); };

            // Menu bar:
            if (ImGui::BeginMenuBar()) {
                defer { ImGui::EndMenuBar(); };
                // Style editor (from the demo window):
                if (ImGui::BeginMenu("Style")) {
                    defer { ImGui::EndMenu(); };
                    ImGui::ShowStyleEditor();
                }
            }

            // Sections:
            if (ImGui::BeginTabBar("Sections")) {
                defer { ImGui::EndTabBar(); };
                if (ImGui::BeginTabItem("About", nullptr, ImGuiTabItemFlags_None)) {
                    defer { ImGui::EndTabItem(); };
                    {
                        ImGui::BeginChild("Scroll");
                        defer { ImGui::EndChild(); };

                        ImGui::TextWrapped("Hello, my name is Walker Williams. I am a very passionate programmer and this is my resume, written in C++, and put on the web for you, recruiter, potential employer, or casual viewer, to look through.");
                        ImGui::NewLine();
                        ImGui::TextWrapped("You might ask yourself: \"Why write your resume in C++?\" The answer is simple. Regular resumes are boring and recruiters and employers go through them day by day looking for certain qualities. I figured the best way to show them that I'm a competent programmer is to program my own resume in a language I am familiar with. Want to know if I can program in C++ right away without spending time looking at my resume? Well guess what? This resume is written in C++ so now you know the answer is yes. This also spices up the recruiter's or employer's life and gives them something they can interact with for once instead of a word document or a pdf they boringly scan for key words, usually with a computer program. It's a nice little surprise to break the day to day monotany.");
                        ImGui::NewLine();
                        ImGui::TextWrapped("Fun fact: This is the first UI I've ever written as a programmer. I learned on the fly just to write this resume.");
                        ImGui::NewLine();
                        ImGui::TextWrapped("PSSSSSSSSSSSSSSSSSSSSST: There is a style button at the top left corner of this resume. Try clicking it and mess with some of the values.");
                        ImGui::NewLine();
                        ImGui::TextWrapped("Click this button to view this resume's codebase on my github (also copies the link to your clipboard):");
                        if (ImGui::Button("Resume Code")) {
                            emscripten_browser_clipboard::copy("https://github.com/WWilliams741/WWilliams741.github.io");
                            open_link_through_clipboard();
                        }
                    }
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
                        if (ImGui::TreeNode("C/C++ (master)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("C++ is my main language besides Jai");
                            IMGUI_BULLETTEXTWRAPPED("This resume is written in C++ thanks to the power of Dear ImGui and Emscripten");
                            IMGUI_BULLETTEXTWRAPPED("I have developed, from scratch solo, critical projects/architecture in C++, with little to no guidance before");
                            {
                                ImGui::Indent();
                                defer { ImGui::Unindent(); };
                                IMGUI_BULLETTEXTWRAPPED("The above projects were all multi-threaded environments in distributed systems");
                            }
                            IMGUI_BULLETTEXTWRAPPED("I was well known at FlexGen Power Systems as one of their best C++ programmers, not even a linter was used without my approval");
                            IMGUI_BULLETTEXTWRAPPED("I know C++11 through C++20, but C++20 and up is my preferred standard");
                            IMGUI_BULLETTEXTWRAPPED("Familiar with the STL, Generic Programming, creating custom allocators/memory management, etc.");
                            IMGUI_BULLETTEXTWRAPPED("Familiar with debugging using Valgrind, GDB, step debugging using breakpoints in an IDE, etc.");
                            IMGUI_BULLETTEXTWRAPPED("Familiar with TDD (Test Driven Development) using Googletest or, my preferred favorite framework, Doctest");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("golang (intermediate)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("I know golang enough to write code in it comfortably, but I still have to look things up every now and then");
                            IMGUI_BULLETTEXTWRAPPED("I have mentored a new software engineer to rewrite a core codebase from scratch in golang");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("python (intermediate)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("I know python at a scripting level");
                            IMGUI_BULLETTEXTWRAPPED("I have written python scripts before in a professional environment");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("bash (intermediate)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("I know bash at a scripting level");
                            IMGUI_BULLETTEXTWRAPPED("I have written bash scripts for an installation/deployment process before professionally");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Jai (intermediate)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Currently a proud member of the closed beta for the ");
                            ImGui::SameLine();
                            if (ImGui::Button("Jai programming language")) {
                                emscripten_browser_clipboard::copy("https://github.com/Jai-Community/Jai-Community-Library/wiki");
                                open_link_through_clipboard();
                            }
                            ImGui::SetItemTooltip("Click this button to get information about the Jai programming language (also copies the link to your clipboard)");
                            IMGUI_BULLETTEXTWRAPPED("Beta access is not easily given out, you have to prove you're worthy of being given access");
                            IMGUI_BULLETTEXTWRAPPED("Only a couple hundred other people are in the beta");
                            IMGUI_BULLETTEXTWRAPPED("Beta is run by Jonathan Blow, creator of best selling games \"Braid\" and \"The Witness\"");
                        }
                        // Practices:
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Agile Development")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Jira boards and filling out tickets for sprints");
                            IMGUI_BULLETTEXTWRAPPED("Markdown documentation on Confluence or Github Wikis");
                            IMGUI_BULLETTEXTWRAPPED("CI (Continuous Integration) using Github PRs (Pull Requests) and Jenkins/AWS (Amazon Web Services)");
                            IMGUI_BULLETTEXTWRAPPED("SCRUM meetings twice a week to check in on progress and potential blockers");
                            IMGUI_BULLETTEXTWRAPPED("Sprints that last 1 month+ with pre-planning");
                            IMGUI_BULLETTEXTWRAPPED("Consistent communication on Slack with team members on a daily basis");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Parallel/Multi-threaded Programming")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Familiar with concepts such as producer-consumer queues (channels), IO pools, mutexes, semaphores, atomics, false sharing, over subscribing, etc.");
                            IMGUI_BULLETTEXTWRAPPED("I know the difference between the two different types of parallel programming paradigms: for performance and for blocking events (like IO)");
                            IMGUI_BULLETTEXTWRAPPED("I have written multi-threaded software before in distributed systems professionally");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Performance Aware Programming")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("I am NOT afraid of memory and low level programming");
                            IMGUI_BULLETTEXTWRAPPED("I am PRO creating custom allocators");
                            IMGUI_BULLETTEXTWRAPPED("I am PRO understanding how your program acesses memory and how that relates to CPU caches and RAM (L1, L2, L3, main memory cache misses and their costs)");
                            IMGUI_BULLETTEXTWRAPPED("I am PRO SUA (Shutup Use Array) 99%% of the time, and the last 1%% is usually a Hash Table");
                            IMGUI_BULLETTEXTWRAPPED("I am PRO writing software from scratch, from an empty main(), and doing many iterations");
                            IMGUI_BULLETTEXTWRAPPED("These above princples I have used in professional environments to great success");
                        }
                        // Environment:
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Software Environment/Tools")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("Docker for simulatating distributed systems and networks, using Dockerfiles and docker-compose for simplicity");
                            IMGUI_BULLETTEXTWRAPPED("Linux environments are my goto (CentOS - Red Hat Linux for example), but Windows is alright");
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
                        if (ImGui::TreeNode("Software Engineer - FlexGen Power Systems (September 2020 -> August 2023)")) {
                            defer { ImGui::TreePop(); };

                            IMGUI_BULLETTEXTWRAPPED("Languages: C/C++, golang, python, bash");
                            IMGUI_BULLETTEXTWRAPPED("Joined FlexGen fresh out of college, my first job, doing 100%% remote work for the first couple months over Slack");
                            IMGUI_BULLETTEXTWRAPPED("FlexGen specializes in BESSs (Battery Energy Storage Systems), a form of distributed system with multiple pieces of hardware in a \"Command and Control\" typology, with thousands of data points in a live system");
                            IMGUI_BULLETTEXTWRAPPED("I developed, along with 3 other people in C++, FlexGen's ESS (Energy Storage System) Controller, their lowest level controller and one of their most important pieces of software");
                            IMGUI_BULLETTEXTWRAPPED("I have written, solo in bash, their entire deployment/installation process, later this was converted to Ansible by another team entirely");
                            IMGUI_BULLETTEXTWRAPPED("I have rewritten, solo in C++, core communication software that is used throughout the entire distributed network, bringing the CPU usage down from 112%% to 1-3%% in our largest use cases and increasing networking performance by about 2-3 times");
                            IMGUI_BULLETTEXTWRAPPED("I have rewritten, solo in C, C++, and golang, the core Unix IPC (Inter Process Communication) architecture that underlied everything, taking a large test that used to take 3 minutes down to 3 seconds, and causing an end-to-end all possible inputs test to go from 1 week down to 1 day or less without breaking anyone's code");
                            {
                                ImGui::Indent();
                                defer { ImGui::Unindent(); };
                                IMGUI_BULLETTEXTWRAPPED("Because of the above achievements FlexGen was able to properly scale to larger sites beyond 100+ MW allowing them to take on some of the largest BESS projects in the world and become a multi-million dollar success story, going from 50 employees to hundreds of employees");
                            }
                            IMGUI_BULLETTEXTWRAPPED("I have mentored a new software engineer to rewrite a core piece of integration architecture from scratch in golang, taking the lines of config from the thousands down to the hundreds and adding programmatic syntax");
                            IMGUI_BULLETTEXTWRAPPED("I was known as one of their best Software Engineers, and could not be given the title of Senior Software Engineer only because I hadn't been there long enough (the lead of software at the time - John Calcagni - said so)");
                            IMGUI_BULLETTEXTWRAPPED("Because of the above achivements I have been given glowing recommendations on my LinkedIn profile from some of the Senior Software Engineers and Managers, everyone respected my expertise so much they wouldn't even allow a C++ linter without my approval");
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
                        if (ImGui::TreeNode("This Resume - Lifetime Active")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("If you're reading this you're viewing this project right now");
                            IMGUI_BULLETTEXTWRAPPED("NOTE: Each time you view this resume things might change, so check back every now and then");
                        }
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
                            IMGUI_BULLETTEXTWRAPPED("Currently a proud member of the closed beta for the ");
                            ImGui::SameLine();
                            if (ImGui::Button("Jai programming language")) {
                                emscripten_browser_clipboard::copy("https://github.com/Jai-Community/Jai-Community-Library/wiki");
                                open_link_through_clipboard();
                            }
                            ImGui::SetItemTooltip("Click this button to get information about the Jai programming language (also copies the link to your clipboard)");
                            IMGUI_BULLETTEXTWRAPPED("Beta access is not easily given out, you have to prove you're worthy of being given access");
                            IMGUI_BULLETTEXTWRAPPED("Only a couple hundred other people are in the beta");
                            IMGUI_BULLETTEXTWRAPPED("Beta is run by Jonathan Blow, creator of best selling games \"Braid\" and \"The Witness\"");
                            IMGUI_BULLETTEXTWRAPPED("I have filled out multiple bug reports across multiple beta versions already");
                            IMGUI_BULLETTEXTWRAPPED("I have contributed to an open source project that the beta members are writing called \"Focus\", an editor written 100%% in Jai that I am using right now to write this resume");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Arduino Uno Simon Say's Game - Completed")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("A simple game with four buttons on a custom built circuit and an Arduino Uno");
                            IMGUI_BULLETTEXTWRAPPED("A random order of lights would be displayed, you would then have to match the light order with the buttons, then another light is added next round");
                            IMGUI_BULLETTEXTWRAPPED("You won if you went 15 rounds following \"Simon's\" orders, and lost if you couldn't memorize them");
                            IMGUI_BULLETTEXTWRAPPED("");
                            ImGui::SameLine();
                            if (ImGui::Button("Code and Demo")) {
                                emscripten_browser_clipboard::copy("https://github.com/WWilliams741/Simon-Says-Game");
                                open_link_through_clipboard();
                            }
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
                            IMGUI_BULLETTEXTWRAPPED("Early Master of Science in Computer Science - GPA: 3.8  /4.0");
                            IMGUI_BULLETTEXTWRAPPED("Bachelor of Science in Computer Science     - GPA: 3.658/4.0");
                            IMGUI_BULLETTEXTWRAPPED("I enrolled in UNCC's Early Master's program");
                            IMGUI_BULLETTEXTWRAPPED("I graduated in 3 years with my Bachelor's and 4 with my Master's");
                        }
                        if (open_action != -1)
                            ImGui::SetNextItemOpen(open_action != 0);
                        if (ImGui::TreeNode("Lee Early College/Central Carolina Community College - (2012 -> 2016)")) {
                            defer { ImGui::TreePop(); };
                            IMGUI_BULLETTEXTWRAPPED("High School Diploma && Associate in Science - GPA: 3.78 /4.0");
                            IMGUI_BULLETTEXTWRAPPED("I was chosen out of Middle School for this High School through an interview process");
                            IMGUI_BULLETTEXTWRAPPED("Students would dual enroll in both High School and Community College courses, graduating from both at the end of 4 years");
                        }
                    }
                }
                // defer macro gift:
                if (ImGui::BeginTabItem("A Gift For You", nullptr, ImGuiTabItemFlags_None)) {
                    defer { ImGui::EndTabItem(); };

                    ImGui::TextWrapped("This is a gift to you, as a way of contributing to your, or the company that you represent's, codebase before even being hired, as a gesture of good will. It is a defer macro for C++11 and beyond. It was used to write this resume's code. Even if you don't hire me at least you got free code out of it.");
                    ImGui::NewLine();

                    if (ImGui::Button("Defer Code")) {
                        emscripten_browser_clipboard::copy("https://github.com/WWilliams741/Utilities/blob/main/defer.hpp");
                        open_link_through_clipboard();
                    }
                    ImGui::SetItemTooltip("Click this button for the defer.hpp file on my github (also copies the link to your clipboard)");
                    ImGui::SameLine();

                    if (ImGui::Button("Live Demo")) {
                        emscripten_browser_clipboard::copy("https://compiler-explorer.com/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGe1wAyeAyYAHI%2BAEaYxBIAnKQADqgKhE4MHt6%2BekkpjgJBIeEsUTFc8XaYDmlCBEzEBBk%2Bfly2mPZ5DDV1BAVhkdFxtrX1jVktCsM9wX3FA2UAlLaoXsTI7BzmAMzByN5YANQmm27IE/ioR9gmGgCC13dm21QMWFT7ACLYAGLYAEoA%2Bm5lMp7ltXtMPt8/oDgaDbgB6eH7G77LxKKheWj7FhMZDEVD7Ij7YCYAj7OoRQjEOoAT32aAORMwqkqXgImHJZIICA5rn2qDeTH2CjQCQ5aOCwH2vxuAElZeSXvsDMV0EwFAA6e6I/YAFQQeAU%2BwS%2BIAbngsEbXtFhTTBExVMK8Cw6Hg6oSCcBPIZgBq9QajYbUejMft%2BMR9sF2cQqLjJdqkQB3QgIfZuZHKWVG7lMMkhTDocmHMxmLDBQjmMz8iOV3aYQxeBKVsNeBhVAQe/ZRelieiF3OEnn7PkC4WizBahFI/VBvH19lGhkcnNk2f0Qw2qKiNEcmnLfboARgDh5zAFztz3Mc2ioAhG0cy%2BX7RPUhJi4iauE3HWhADyuuwEB/Q5a0IxxPECSDdUFB8c8iW7SsGFQZk1gSAhmwiOlXiYTECEnb8kX2IjiLBTAaBCSEfgBAB1WVdQACX%2BbAAA03GwZRdVlX9QiEckIlQU0OUrUCNQQN9myJXtUETYdVFQjpswJbtuXxRMGATYjNOCFIDlA4VanZNhBCNCAxG5ZZgFTBVE2WWhCwNSzaDpJh0HNJRyWAJhtNXAh5lBR48CoV5KOhWiGKY1j2M47ihH8zZwQoz4qP%2BP8WLYjj9iQlDMDQ6gxCUPzbi2NolDihKOSS6FUsijKsrknKCAgAhiC8TBCoeTZXECr8tkC/Z/n%2BZAEm8BRhrRQ5NiuTZ3n2MwNC4AB2DRNgCL8dXeMibXA/F9ggS4jhmtxrGsRb5hAe5DOGq8jjcAgaTFZg2DTDBMH224JhahwPk2msFqsW5NI8A4lyOf6CPpAQJmZE1hUbJJ6l2/bprTY7LDms64r6gahpGsajSRma5rmjQzFWgHiLQBgodUGGNqoaIICBjkIhvZAAGt5hCgFqvS3V9iApcIBZ1B2c5kw/vF957k0ynqZhgA/OmGc5yrud/NKosOP6IawCA/M2CwtalorHhK17yaIpXiEZl6u1ZjmuZS9War5gWXqF%2B2xYlhbjZRYjFZ%2BvXHZ5zXxcNwX9cNyW4q6qhQR90GyrIiEAHFsFCf5VZSm4AFlsH%2BeibjcABpCBHswUgIdbaNOfL4tNi2KvBGiJPyI5NOM6z0Jc%2BwMvWArpua/2DvM6hbme4LovS/Lyu0Gr6J2tItuDx%2B776YjEeu57iB/lA/5K4GtxfzkUIAIBf4xeRkwAFYrDMAA2G/3iDrvnd5nrTdoUqiqnNettxHae1JoHRRpYE6XB2qXQMOyG6d0Hr92elgN6NwPpeC%2BlbLWYNAa2xBgbaWxErY2wOMLUWwc36azdrrEhDsw7RwtvsAO68X5jydhrDKYcdaYD1qDI28cpZ4Pes1NBZIrb/BTowaIuYSCYMJJgFgV0YHHDgYwBBTNLh/2IDdNRk1%2BTvikTWSwRCORLhViwkO7DtbEFJCsBgGitEvX2n9EGCcDZG14QIjq5Vh7p1HslbuedJ4lz7mwWeyxm7EFrggrYjc57hNbqnHxW887BIHrEoem8zET0LkEmeg8F7xIonpHCRIMl%2BO3rvH6%2B9%2BqAmPqfaEF8JozVEeIkI1IiCaO9jNawhxb7mEftfZ%2Bpi/HkI4h/TqLxuo/3BtgVQrBhrigUEwEk51f7FIJAoVAbB/gYjbKZNkFwH6RgYAkNkXswY6k0npDhqMLDBBOehVx8IABU%2BxlBsk4XbEWbN9g8isZ2ZkrJ2ScmHEqUcIpUBikrnuLwPZbFWPXp2Ssdy2TNmCIOWcttqHKjwGzDkSFiA4ixE8pE0cPEXOIjqe5/JuQ2iXD86Iu59yJkMFyAkALkBsmZmREgIFV60BxXikghKNKaSsQQGxRz7mJ1/nQ6Zsz5H0GxF5WxEAFAIFsoWE0UZ9hcC7Nyv5ZgMa/21TiYIQdaG/0uavDhWrBBUD2iWHktAbyVz0jZYgdkwBgFmjfNw6kSyR3cecwixErna1tQQe1lYnUupXgi91nrvVcF9f6w1PDSXBt4VM4qEy47ZtuCa5V5qJb0LDYbCNUbHVtFjW6kgiafXXz9ZWQNGb8FETLcaYgUZK1mBjagV1q8E3oC9TqlNzb00uLBrKjgixaCcGvrwPwHAtCkFQJwI6YDLDCmWKsISjweCkAIJoGdiweQuQGHrUgbMQDXw0PoTgkhF3HtXZwXgCgQB3qPcumdpA4CwBgIgFAmyEh0GiOQSgaB5GgZiMALgmwzCkCwOaNYAA1PAmBEy/gepwA9NBaDRnfULZ9lJmDEBpDh3gJHaS/giNoSoX6D2QaMgQX8DAnLPqwBELwwA3C9nfdwXgWAcRGHEN%2BxDeArFVEEvxld7LOXPqjG0Z9/KIjUjI0zZ9zVnQUcWFQAwwAFBoYw1hxgFGZCCBEGIdgUhzPyCUGoZ9ugWgGCMCgG5%2Bg8ARHfZARYEKFKcAALRnAOqYTdFg5r7AC7%2BKsAXqK9ki9RZkzUhSxbFGqQQeBkAJcpoJD8aQEtqpctJSLzxUABeymhNICheACWiF2y08BFgVHbH4CArhRjNFIIEaYRQSjZGSKkAQHX%2Bu5DSL0XrcxWjtGqJMYb4w2j0Zm90cb/RShDG6HN9b9QVuzFKE1ndawJCzvnU%2BsTa6OD7FUAADnvgF%2B%2BkhiTICy3BjUVYIC4EINIrYEDeBfq0PMRY1775mA1Nfe9HBH2kBYDeu9S6V3nbfR%2Bw9x7Fh/sA8sAg9zwMQEgyB%2BgxBQj904Nd2793HvPc2K93gBZPv1b0PwCzohxA2YZ3ZlQ6gxNOdIC%2BJgCQdPg4XaQOHNXOC/jZFS0cJO7sPeAE9nVlO3seCg/j%2BuP3kffoB6QM9WAYiXrnRD3g0Pb1C%2BfQj2wSO/snqvTD8HmxTvw9fer/7x2OBmHtyLjgv2UeLFyykZwkggA%3D%3D");
                        open_link_through_clipboard();
                    }
                    ImGui::SetItemTooltip("Click this button for an interactive demo of defer.hpp (also copies the link to your clipboard)");
                    ImGui::Separator();
                    {
                        ImGui::BeginChild("Scroll");
                        defer { ImGui::EndChild(); };

                        ImGui::TextWrapped(defer_macro_code);
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
