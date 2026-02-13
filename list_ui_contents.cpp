#include "src/wz/WzResMan.h"
#include <iostream>

int main()
{
    auto& resMan = ms::WzResMan::GetInstance();
    resMan.SetBasePath("resources/Data");

    if (!resMan.LoadWzFile("UI"))
    {
        std::cout << "Failed to load UI.wz" << std::endl;
        return 1;
    }

    // Get UI root
    auto uiProp = resMan.GetProperty("UI");
    if (!uiProp)
    {
        std::cout << "UI root not found" << std::endl;
        return 1;
    }

    std::cout << "UI.wz contents:" << std::endl;
    std::cout << "===============" << std::endl;

    for (const auto& [name, child] : uiProp->GetChildren())
    {
        std::cout << "UI/" << name;
        if (child->HasChildren())
        {
            std::cout << " (" << child->GetChildCount() << " children)";
        }
        std::cout << std::endl;
    }

    return 0;
}
