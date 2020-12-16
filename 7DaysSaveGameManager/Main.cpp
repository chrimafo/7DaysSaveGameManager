// HelloWindowsDesktop.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <tinyxml2.h>
#include <filesystem>
#include <ctime>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
namespace fs = std::filesystem;

// The main window class name.
static TCHAR szWindowClass[] = _T("7 Days to die Save Game Manager");
// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("7 Days to die Save Game Manager");
// The handles
HINSTANCE hInst;
// The IDs
constexpr auto ID_BTN = 0;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SaveLastPlayedGame();


int CALLBACK WinMain(
_In_ HINSTANCE hInstance,
_In_opt_ HINSTANCE hPrevInstance,
_In_ LPSTR     lpCmdLine,
_In_ int       nCmdShow
)
{    
    WNDCLASSEX wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    hInst = hInstance;

    RECT desktop;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &desktop, 0);
    const int width = 200 + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
    const int height = 82 + GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        (WS_OVERLAPPEDWINDOW) & ((0UL - 1) ^ WS_THICKFRAME),  // not resizable
        desktop.right - width + GetSystemMetrics(SM_CXSIZEFRAME) * 2,
        desktop.bottom - height + GetSystemMetrics(SM_CYSIZEFRAME) * 2,
        width,
        height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            szWindowClass,
            NULL);

        return 1;
    }

    HWND hwndButton = CreateWindow(
        L"BUTTON",
        L"Save last played",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        25,
        25,
        150 - GetSystemMetrics(SM_CXSIZEFRAME) * 2,
        32 - GetSystemMetrics(SM_CYSIZEFRAME) * 2,
        hWnd,
        (HMENU)ID_BTN,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_COMMAND:
        if (((HWND)lParam) && (HIWORD(wParam) == BN_CLICKED))
        {
            int iMID;
            iMID = LOWORD(wParam);
            switch (iMID)
            {
            case ID_BTN:
                SaveLastPlayedGame();
                break;
            }
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

void SaveLastPlayedGame()
{
    string newestGame = "";
    string newestWorld = "";
    string newestGameName = "";
    tm newestTime{};

    char* pValue;
    size_t len;
    errno_t err = _dupenv_s(&pValue, &len, "APPDATA");
    string appdataPath = string(pValue);
    fs::path basePath{ (appdataPath + string("\\7DaysToDie\\Saves")).c_str() };

    for (const auto& world : fs::directory_iterator(basePath))
    {
        const auto folderName = world.path().filename().string();
        if (world.is_directory())
        {
            for (const auto& game : fs::directory_iterator(world.path()))
            {
                if (!game.is_directory() || game.path().filename().string()._Equal("Saves") || game.path().filename().string()._Equal("Deaths"))
                {
                    continue;
                }
                for (const auto& file : fs::directory_iterator(game.path()))
                {
                    if (file.is_directory() || !file.path().filename().string()._Equal("players.xml"))
                    {
                        continue;
                    }

                    // load save time
                    tinyxml2::XMLDocument doc;
                    doc.LoadFile(file.path().string().c_str());
                    const char* lastLogin = doc.FirstChildElement("persistentplayerdata")->FirstChildElement("player")->FindAttribute("lastlogin")->Value();
                    tm currentTime = boost::posix_time::to_tm(boost::posix_time::time_from_string(lastLogin));
                    if (newestGame.empty() || difftime(mktime(&currentTime), mktime(&newestTime)) > 0.0)
                    {
                        newestTime = currentTime;
                        newestGame = game.path().string();
                        newestWorld = world.path().string();
                        newestGameName = game.path().filename().string();
                    }
                }
            }
        }
    }

    // find latest save game
    string newestGameSavesFolder = newestWorld + string("\\Saves");
    fs::path savesFolder{ newestGameSavesFolder.c_str() };
    int newestSaveGame = 0;

    for (const auto& world : fs::directory_iterator(savesFolder))
    {
        const auto folderName = world.path().filename().string();
        if (folderName.find(newestGameName) != string::npos)
        {
            try
            {
                int saveNumber = stoi(folderName.substr(newestGameName.length() + 1));
                if (saveNumber > newestSaveGame)
                {
                    newestSaveGame = saveNumber;
                }
            }
            catch(exception &_)
            {
                
            }
        }
    }

    // save folder to next free spot
    fs::path targetFolder{ savesFolder.string() + string("\\") + newestGameName + string("_") + to_string(newestSaveGame + 1) };
    fs::copy(newestGame, targetFolder, fs::copy_options::recursive);

    string message{ "Successfully saved progress for " };
    message.append(newestGameName);
    
    MessageBoxA(NULL, message.c_str(), "Success", MB_OK);
}