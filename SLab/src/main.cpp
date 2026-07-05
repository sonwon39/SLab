#include "SimpleApp.h"
#include <iostream>
#include "Utility.h"

int main()
{
    try
    {
        Core::SimpleApp app(1280, 720, 0);
        if (app.Initialize())
        {
            return app.Run();
        }
    }
    catch (const DxException& e)
    {
        std::wcout << L"[DxException] " << e.ToString() << std::endl;
        return -1;
    }
    catch (const std::exception& e)
    {
        std::cout << "[exception] " << e.what() << std::endl;
        return -1;
    }
}
