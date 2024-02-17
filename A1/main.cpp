/***********************************************************
* author:len mudgett
* email: lmudgett@gmail.com
* course: COMP 4300 
* assignment: 1
* signature:
* NOTES:
    compiled under vs 2022, SFML v2.6.1 and ImGUI 1.90.4
    all information provided is from the lectures on youtube by Prof David Churchill @ Memorial University COMP 4300.
	    i did not attend the course which means I did not have access to the provided sample code(s) or material(s).
	    everything done is an approximation based on the lectures and requirements.
	    i decided to take this on as a fun project to see if i could complete the assignments solely using the lectures.

    requirements:
        * read a config file from the program's execution location. 
            this file will define a single window, a single font, multiply circles and multiply rectangles.
            the config file will be in ASCII format and tokens will be separated by SPACE character.
            order of items define does not matter
            ^enhancements error handling has been applied and a command line argument is available to read a config file from another location
            ^enahcnements depending on the OS the programe is compiled to the pathing characters will be adjusted when read from the config file
            ** FONT DEFINATION TOKEN ORDER : type / file name / size / r / g / b
            ** WINDOW DEFINATION TOKEN ORDER : type / width / height
            ** CICLE TOKEN ORDER : type / label / x / y / sx / sy / r / g / b / radius
            ** RECTANGLE TOKEN ORDER : type / label / x / y / sx / sy / r / g / b / w / h
        * the program will display the read shapes to the screen and have the colide off the screen edges based on defined window
        * the drawn shape will display its label in it's center
        * a gui will be displayed allowing for the modification of the selected shape
        * the gui will allow for:
            ** the shape to be shown or hidden via checkbox
            ** the label shown or hidden via checkbox
            ** the shape to be scaled via a slider
            ** both the x and y velocity to be changed via sliders
            ** the shapes color to be changed via color picker
            ** the shape's position to be reset to its original position via button
            ** the shape's label to be changed  via textbox and button
           
        TODOS:
            * BUG: before scaling there should be a check to make sure the shape isn't larger that 90% of the window
            * BUG: when scaling the shape should not be allow to go outsize the window 
***********************************************************/
#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

const std::string DEFAULT_CONFIG_FILE_NAME = "config.txt";
const int NAME_SIZE = 255; //256 standard char size for a textbox;

//having the configurtion type as an enum provides some strictness to the types that can be defined in the file as well gives an option for error handling unknow types
enum ConfigType {
    window,
    font,
    circle,
    rectangle,
    unknown
};

ConfigType static toConfigType(const std::string s) {
    if (s == "Window") {
        return ConfigType::window;
    }
    else if (s == "Font") {
        return ConfigType::font;
    }
    else if (s == "Rectangle") {
        return ConfigType::rectangle;
    }
    else if (s == "Circle") {
        return ConfigType::circle;
    }
    else {
        return ConfigType::unknown;
    }
}

struct Window {
    std::string name;
    int w;
    int h;
};

struct Font {
    std::string path;
    sf::Font font;
    int size;
    int r, g, b;
};

/*
abstract shape class
*/
class Shape {
private:
public:
    virtual ~Shape() = default;
    virtual void draw(sf::RenderWindow& window) const = 0; //tightly coupling the class to render but at the momment seems best for optimization
    virtual void setColor(int r, int g, int b) = 0;
    virtual void setPosition(float x, float y) = 0;
    virtual sf::Vector2f getPosition() = 0;
    virtual sf::Vector2f getSize() = 0;
    virtual float getScale() = 0;
    
    int id;
    float orgX, orgY;
    char* name;
    float sx, sy;
    float scale;
    bool drawable;
    bool drawLabel;
    sf::Text label;
    float color[3];
};

class Circle : public Shape {
private:
    sf::CircleShape circle;
public:
    Circle(std::string name, float x, float y, float radius, int r, int g, int b) {
        orgX = x, orgY = y;
        scale = 1.f;
        drawLabel = true;
        color[0] = (float)r / 255, color[1] = (float)g / 255, color[2] = (float)b / 255; // have to convert the color to dec for coloredit to work correctly
        label = sf::Text();
        circle = sf::CircleShape(radius);
        circle.setFillColor(sf::Color(r, g, b));
        circle.setPosition(sf::Vector2f(x, y));
        this->name = new char[NAME_SIZE];
        strcpy_s(this->name, name.size() + 1, name.c_str());
    }
    ~Circle() {
        //delete[] name; //supposedly unique_ptr takes care of this
        std::cout << "cirle of id:= " << id << " is destroyed" << std::endl;
    }
    sf::Vector2f getPosition() override {
        return circle.getPosition();
    }
    sf::Vector2f getSize() override {
        float size = (circle.getRadius() * 2.f) * circle.getScale().x;
        return sf::Vector2f(size, size);
    }
    void setColor(int r, int g, int b) override {
        circle.setFillColor(sf::Color(r, g, b));
    }
    void setPosition(float x, float y) override {
        circle.setPosition(sf::Vector2f(x, y));
    }
    float getScale() override {
        circle.setScale(scale, scale);
        return circle.getScale().x; //only scaling by 1 value for both x and y
    }
    void draw(sf::RenderWindow& window) const override {
        window.draw(circle);
    }
};

class Rectangle : public Shape {
private:
    sf::RectangleShape rectangle;
public:
    Rectangle(std::string name, float x, float y, float w, float h, int r, int g, int b) {
        orgX = x, orgY = y;
        scale = 1.f;
        drawLabel = true;
        color[0] = (float)r / 255, color[1] = (float)g / 255, color[2] = (float)b / 255; // have to convert the color to dec for coloredit to work correctly
        label = sf::Text();
        rectangle = sf::RectangleShape(sf::Vector2(w, h));
        rectangle.setFillColor(sf::Color(r, g, b));
        this->name = new char[NAME_SIZE];
        strcpy_s(this->name, name.size() + 1, name.c_str());
    }
    ~Rectangle() {
        //delete[] name; //supposedly unique_ptr takes care of this
        std::cout << "rectangle of id:= " << id << " is destroyed" << std::endl;
    }
    sf::Vector2f getPosition() override {
        return rectangle.getPosition();
    }
    sf::Vector2f getSize() override  {
        sf::Vector2f size = rectangle.getSize(); //this is kinda dumb when scaled the size should be automatically adjusted
        size.x *= rectangle.getScale().x;
        size.y *= rectangle.getScale().y;
        return size;
    }
    void setColor(int r, int g, int b) override {
        rectangle.setFillColor(sf::Color(r, g, b));
    }
    void setPosition(float x, float y) override {
        rectangle.setPosition(sf::Vector2f(x, y));
    }
    float getScale() override {
        rectangle.setScale(scale, scale);
        return rectangle.getScale().x; //only scaling by 1 value for both x and y
    }
    void draw(sf::RenderWindow& window) const override {
        window.draw(rectangle);
    }
};

void static readConfig(const std::string filePath, Window& w, std::vector<std::unique_ptr<Shape>>& shapes,
    Font &f) {

    if (!fs::exists(filePath) && fs::is_regular_file(filePath) || !fs::is_regular_file(filePath)) {
        std::ostringstream err;
        err << "passed config file '" << filePath << "' either doesn't exist or is not a file";
        throw std::runtime_error(err.str());
    }

    std::ifstream stream(filePath);

    if (!stream.is_open()) {
        std::ostringstream err;
        err << "unable to open passed config file '" << filePath << "'";
        throw std::runtime_error(err.str());
    }

    stream.seekg(0, std::ios::end);
    if (stream.tellg() == 0) {
        stream.close();
        std::ostringstream err;
        err << "passed config file '" << filePath << "' is empty";
        throw std::runtime_error(err.str());
    }
    else {
        stream.seekg(0, std::ios::beg); //put the cursor back to start so we can read the file
    }

    std::string line;
    bool winDef = false; //only 1 window allowed
    bool fontDef = false; //only 1 font allowed
    int id = 0;
    while (std::getline(stream, line)) {
        //std::cout << line << std::endl;//debug only 
        //only process lines that have content
        if (line != "") {
            std::istringstream iss(line);
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) {
                tokens.push_back(token);
            }

            switch (toConfigType(tokens[0])) {
            case window: {
                if (tokens.size() != 3) {
                    std::cerr << "WARNING: the following window defination '" << line
                        << "' has errors, expecting at least 3 tokens got " << tokens.size() << " so skipping!" << std::endl;
                    break;
                }
                if (winDef) {
                    std::cerr << "WARNING: only 1 window configuration line allowed, ignoring line '" << line << "'" << std::endl;
                    break;
                }
                try {
                    w = {"COMP4300 A1 LDM", std::stoi(tokens[1]), std::stoi(tokens[2])};
                    //w.name = tokens[3];
                    winDef = true;
                }
                catch (const std::exception& e) {
                    std::cerr << "WARNING: unable to parse window defination '" << line << "' so skipping, details: " << e.what() << std::endl;
                }
                break;
            }
            case font: {
                //token order: file name / size / r / g / b
                if (tokens.size() != 6) {
                    std::cerr << "WARNING: the following font defination '" << line
                        << "' has errors, expecting at least 6 tokens got " << tokens.size() << " so skipping!" << std::endl;
                    break;
                }
                if (fontDef) {
                    std::cerr << "WARNING: only 1 font configuration line allowed, ignoring line '" << line << "'" << std::endl;
                    break;
                }
                try {
#ifdef _WIN32 //fix pathing for windows machines
                    size_t contains = tokens[1].find('/');
                    if (contains != std::string::npos) {
                        tokens[1].replace(contains, 1, "\\");
                    }
#else //linux machine path corrections *NOTE wasn't tested since i didnt have the time to spin up linux vm and work on a make file
                    size_t contains = tokens[1].find('\');
                    if (contains != std::string::npos) {
                        tokens[1].replace(contains, 1, "/");
                    }
#endif 
                    sf::Font ff;
                    if (!ff.loadFromFile(tokens[1])) {
                        throw std::runtime_error("unable able to load font ");
                    }
                    f = { tokens[1], ff,  std::stoi(tokens[2]), std::stoi(tokens[3]), std::stoi(tokens[4]), std::stoi(tokens[5]) };
                    fontDef = true;
                }
                catch (const std::exception& e) {
                    std::cerr << "WARNING: unable to parse font defination '" << line << "' so skipping, details: " << e.what() << std::endl;
                }                
                break;
            }
            case circle: {
                //token order: label / x / y / sx / sy / r / g / b / radius
                if (tokens.size() != 10) {
                    std::cerr << "WARNING: the following circle defination '" << line
                        << "' has errors, expecting at least 10 tokens got " << tokens.size() << " so skipping!" << std::endl;
                    break;
                }
                try {
                    auto s = std::make_unique<Circle>(tokens[1], std::stof(tokens[2]), std::stof(tokens[3]),
                        std::stof(tokens[9]), std::stoi(tokens[6]), std::stoi(tokens[7]), std::stoi(tokens[8]));
                    s->id = id;
                    s->sx = std::stof(tokens[4]);
                    s->sy = std::stof(tokens[5]);
                    s->drawable = true;
                    shapes.push_back(std::move(s));
                }
                catch (const std::exception& e) {
                    std::cerr << "WARNING: unable to parse circle defination '" << line << "' so skipping, details: " << e.what() << std::endl;
                }
                break;
            }
            case rectangle: {
                //token order: label / x / y / sx / sy / r / g / b / w / h
                if (tokens.size() != 11) {
                    std::cerr << "WARNING: the following rectangle defination '" << line
                        << "' has errors, expecting at least 11 tokens got " << tokens.size() << " so skipping!" << std::endl;
                    break;
                }
                try {
                    auto s = std::make_unique<Rectangle>(tokens[1], std::stof(tokens[2]), std::stof(tokens[3]), std::stof(tokens[9]), std::stof(tokens[10]),
                        std::stoi(tokens[6]), std::stoi(tokens[7]), std::stoi(tokens[8]));
                    s->id = id;
                    s->sx = std::stof(tokens[4]);
                    s->sy = std::stof(tokens[5]);
                    s->drawable = true;
                    shapes.push_back(std::move(s));
                }
                catch (const std::exception& e) {
                    std::cerr << "WARNING: unable to parse rectangle defination '" << line << "' so skipping, details " << e.what() << std::endl;
                }
                break;
            }
            default: {
                std::cerr << "WARNING: the following line '" << line
                    << "' has no class defination so skipping!" << std::endl;
                break;
            }
            }
            id++;//dirty id generation 
        }
    }
    stream.close();

    if (!fontDef) {
        throw std::runtime_error("unable to find at least 1 valid font, you must define at least 1 font in the config file!");
    }

    if (!winDef) {
        w.w = 1280;
        w.h = 720;
        w.name = "COMP4300 A1 LDM";
        std::cerr << "WARNING: no window defination found in config file, using default values 1280 720 COMP4300 A1" << std::endl;
    }
}

/*
decoupled movement from shapes
*/
void static moveShape(const Window w, std::unique_ptr<Shape>& shape) {
    auto curPos = shape->getPosition();

    float dx = curPos.x + shape->sx;
    float dy = curPos.y + shape->sy;
    if (dx < 0 || dx > (w.w - shape->getSize().x)) {
        shape->sx *= -1;
    }
    if (dy < 0 || dy > (w.h - shape->getSize().y)) {
        shape->sy *= -1;
    }

    shape->setPosition(dx, dy);
}

/*
center a sf text within the position of the provided object
*/
void static centerText(sf::Text& text, const sf::Vector2f shapeSize, const sf::Vector2f shapePosition, const float scaleFactor) {
    auto lc = text.getGlobalBounds().getSize() / 2.f;                           //label center
    auto lb = lc + text.getLocalBounds().getPosition();                         //local bounds
    auto ss = shapeSize / 2.f;                                                  //half the size of shape
    auto sc = sf::Vector2f(shapePosition.x + ss.x, shapePosition.y + ss.y);     //adjust the label position according to the size of the shape
    text.setOrigin(lb);
    text.setPosition(sc);
}

int main(int argc, char** argv)
{
    std::string configFile;
    char* txtBuffer = new char[NAME_SIZE];
    try {
        //rudimentary args handler 
        if (argc == 1) {
            std::cout << "INFO: Reading from default location and file name for config file './config.txt'" << std::endl;
            configFile = DEFAULT_CONFIG_FILE_NAME;
        }
        else {
            if (argc == 3 && std::strcmp(argv[1], "-config") == 0) {
                std::cout << "INFO: reading config file from " << argv[2] << std::endl;
                configFile = argv[2];
            }
            else {
                std::cout << "Program accepts the following arguments" << std::endl;
                std::cout << "      -?? displays information of the program" << std::endl;
                std::cout << "      -config $PATH_TO_CONFIG_FILE will run a config file from specified path location" << std::endl;
                return 0;
            }
        }

        //attempt to start the demo
        Window win;
        Font font;
        std::vector<std::unique_ptr<Shape>> shapes;

        readConfig(configFile, win, shapes, font);

        //populate the shapes with their text, it has to be done here since the position of the font isn't guaranteed in the config file
        for (auto& shape : shapes) {
            shape->label.setString(shape->name);
            shape->label.setFont(font.font);
            shape->label.setCharacterSize(font.size);
            shape->label.setFillColor(sf::Color(font.r, font.g, font.b));
        }

        sf::RenderWindow window(sf::VideoMode(win.w, win.h), win.name);
        ImGui::SFML::Init(window);
#ifdef DEV // set the font scale higher for my monitor
        auto& gio = ImGui::GetIO();
        gio.FontGlobalScale = 2.f;
#endif 
        window.setFramerateLimit(60);
        const float vSliderSize = 292.f;
        int selectedIndex = 0;
        sf::Clock deltaClock;
        
        if (!shapes.empty()) {
            strcpy_s(txtBuffer, NAME_SIZE, shapes[selectedIndex]->name); //if not empty set the textbox with the name of the shape
        }

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                ImGui::SFML::ProcessEvent(event);
                if (event.type == sf::Event::Closed)
                    window.close();
            }
            if (!shapes.empty()) {
                ImGui::SFML::Update(window, deltaClock.restart());
                ImGui::Begin("Modify Shape");
                if (ImGui::BeginCombo("Shape", shapes[selectedIndex]->name)) {
                    for (int i = 0; i < shapes.size(); i++) {
                        const bool isSelected = (selectedIndex == i);
                        if (ImGui::Selectable(shapes[i]->name, isSelected)) {
                            selectedIndex = i;
                            strcpy_s(txtBuffer, NAME_SIZE, shapes[selectedIndex]->name);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }   
                    }
                    ImGui::EndCombo();
                }
                ImGui::Checkbox("Draw Shape", &shapes[selectedIndex]->drawable);
                ImGui::SameLine();
                ImGui::Checkbox("Draw Label", &shapes[selectedIndex]->drawLabel);
                ImGui::SliderFloat("Scale", &shapes[selectedIndex]->scale, 0.0f, 10.0f);
                ImGui::SetNextItemWidth(vSliderSize);
                ImGui::SliderFloat("##v1", &shapes[selectedIndex]->sx, -5.0f, 5.0f);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(vSliderSize);
                ImGui::SliderFloat("Velocity", &shapes[selectedIndex]->sy, -5.0f, 5.0f);
                if (ImGui::ColorEdit4("Color", &shapes[selectedIndex]->color[0], ImGuiColorEditFlags_NoAlpha)) {
                    shapes[selectedIndex]->setColor(
                        static_cast<int>(shapes[selectedIndex]->color[0] * 255.0f),
                        static_cast<int>(shapes[selectedIndex]->color[1] * 255.0f),
                        static_cast<int>(shapes[selectedIndex]->color[2] * 255.0f)
                    );
                }
                ImGui::InputText("Label", txtBuffer, NAME_SIZE);
                if (ImGui::Button("Set Label")) {
                    strcpy_s(shapes[selectedIndex]->name, NAME_SIZE, txtBuffer);
                    shapes[selectedIndex]->label.setString(shapes[selectedIndex]->name);
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset Shape")) {
                    shapes[selectedIndex]->setPosition(shapes[selectedIndex]->orgX, shapes[selectedIndex]->orgY);
                }
                ImGui::End();

                window.clear();
                
                for (auto& shape : shapes) {
                    if (shape->drawable) {
                        shape->draw(window);
                        centerText(shape->label, shape->getSize(), shape->getPosition(), shape->getScale()); //implied functionality, without this call the scale will not work
                        if (shape->drawLabel) {
                            window.draw(shape->label);
                        }
                        //move the shape
                        moveShape(win, shape);
                    }
                }
                ImGui::SFML::Render(window);
            }
            else {
                auto label = sf::Text();
                label.setFont(font.font);
                label.setCharacterSize(48);
                label.setString("Hey buddy, looks like you have no shapes in your\n config file, better go fix it or this is going to\n be a boring demo!");
                label.setFillColor(sf::Color::Red);
                centerText(label, sf::Vector2f((float)win.w, (float)win.h), sf::Vector2f(0, 0), 1);
                window.draw(label);
            }
            
            window.display();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: an unrecoverable exception occured, details: " << e.what() << std::endl;
        return 1;
    }
    delete[] txtBuffer;
    //std::cout << "config file read success" << std::endl; //debug only
    ImGui::SFML::Shutdown();
    return 0;
}