#include "GameUI.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace {
const glm::vec4 cream(1.0f, .94f, .79f, 1);
const glm::vec4 green(.55f, .88f, .46f, 1);
}

void GameUI::Init() { m_Field.Init(); Enter(Screen::Main); }
void GameUI::Enter(Screen s) {
    m_Screen = s; m_Time = 0; m_Fade = .25f; m_Focused = 0;
    m_Field.UpdateMouse(-1, -1);
    if (s == Screen::Playing) m_Field.UpdateMouse(m_MouseX, m_MouseY);
}
void GameUI::Restart() { m_Field.Init(); Enter(Screen::Playing); }
void GameUI::Update(float dt) {
    dt = std::max(0.0f, std::min(dt, .1f));
    m_Time += dt; m_Fade = std::max(0.0f, m_Fade - dt);
    if (m_Screen == Screen::Intro && m_Time >= 15) { Restart(); return; }
    if (m_Screen != Screen::Playing) return;
    m_Field.Update(dt);
    const auto state = m_Field.GetGameLogic().GetGameState();
    if (state == STATE_VICTORY) Enter(Screen::Won);
    else if (state == STATE_GAME_OVER) Enter(Screen::Lost);
}
int GameUI::ButtonCount() const { return m_Screen == Screen::Paused ? 3 : 2; }
int GameUI::HoveredButton() const {
    if (m_MouseX < 430 || m_MouseX > 850) return -1;
    for (int i = 0; i < ButtonCount(); ++i)
        if (m_MouseY >= 330 + i * 76 && m_MouseY <= 388 + i * 76) return i;
    return -1;
}
void GameUI::Mouse(float x, float y) {
    const bool moved = x != m_MouseX || y != m_MouseY;
    m_MouseX = x; m_MouseY = y;
    if (m_Screen == Screen::Playing) m_Field.UpdateMouse(x, y);
    else if (moved && HoveredButton() >= 0) m_Focused = HoveredButton();
}
void GameUI::Activate(int index) {
    if (m_Screen == Screen::Main) {
        if (index == 0) { m_Field.Init(); Enter(Screen::Intro); }
        else m_Quit = true;
    } else if (m_Screen == Screen::Paused) {
        if (index == 0) Enter(Screen::Playing);
        else if (index == 1) Restart();
        else Enter(Screen::Main);
    } else if (m_Screen == Screen::Won || m_Screen == Screen::Lost) {
        if (index == 0) Restart(); else Enter(Screen::Main);
    }
}
void GameUI::Click(int button, int action) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    if (m_Screen == Screen::Playing) m_Field.OnMouseClick(button, action);
    else if (m_Screen != Screen::Intro && HoveredButton() >= 0) Activate(HoveredButton());
}
void GameUI::Key(int key, int action) {
    if (action != GLFW_PRESS) return;
    if (m_Screen == Screen::Playing) {
        if (key == GLFW_KEY_ESCAPE) Enter(Screen::Paused);
        else if (key == GLFW_KEY_R) Restart();
        else m_Field.OnKey(key, action);
        return;
    }
    if (m_Screen == Screen::Intro) {
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER) Restart();
        else if (key == GLFW_KEY_ESCAPE) Enter(Screen::Main);
        return;
    }
    if (m_Screen == Screen::Paused && key == GLFW_KEY_ESCAPE) { Enter(Screen::Playing); return; }
    if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) m_Focused = (m_Focused + 1) % ButtonCount();
    else if (key == GLFW_KEY_UP || key == GLFW_KEY_W) m_Focused = (m_Focused + ButtonCount() - 1) % ButtonCount();
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) Activate(m_Focused);
}
void GameUI::FocusLost() { if (m_Screen == Screen::Playing) Enter(Screen::Paused); }
void GameUI::CenterText(Renderer2D& r, const std::string& text, float y, float scale, const glm::vec4& color) {
    r.DrawText(text, (1280 - r.GetTextWidth(text, scale)) / 2, y, scale, color);
}
void GameUI::RenderIntro(Renderer2D& r) {
    r.DrawQuad(0, 0, 1280, 720, glm::vec4(.16f,.22f,.20f,1));
    for (int x = 0; x < 1280; x += 80) r.DrawLine({float(x),90},{float(x),480},1,glm::vec4(.24f,.31f,.27f,1));
    r.DrawQuad(0, 480, 1280, 240, glm::vec4(.64f,.46f,.29f,1));
    // Home fridge, with separate doors, handles and food visible in its window.
    r.DrawQuad(75,180,225,340,glm::vec4(.71f,.77f,.73f,1));
    r.DrawRectOutline(85,190,205,135,4,cream);
    r.DrawRectOutline(85,335,205,175,4,cream);
    r.DrawQuad(105,355,165,100,glm::vec4(.23f,.30f,.25f,1));
    for (int i=0;i<3;++i) { r.DrawQuad(115+i*48,400,34,35,glm::vec4(.93f,.67f,.30f,1)); r.DrawCircle({132.0f+i*48,400},17,cream); }
    r.DrawQuad(275,240,6,55,cream); r.DrawQuad(275,365,6,55,cream);
    r.DrawText("HOME FRIDGE",90,150,1.6f,cream);
    float chefX = 340 + std::min(m_Time/4,1.0f)*160;
    float bob = std::sin(m_Time*7)*3;
    // Chef walks in, raises a spatula and faces the invading ant.
    r.DrawQuad(chefX-24,380+bob,48,75,cream);
    r.DrawCircle({chefX,355+bob},25,glm::vec4(.96f,.73f,.52f,1));
    for(int i=-1;i<=1;++i) r.DrawCircle({chefX+i*18,322+bob},20,cream);
    r.DrawQuad(chefX-27,325+bob,54,15,cream);
    r.DrawCircle({chefX+10,353+bob},3,glm::vec4(.08f,.10f,.08f,1));
    r.DrawLine({chefX-13,455+bob},{chefX-18,490},10,cream);
    r.DrawLine({chefX+13,455+bob},{chefX+22,490},10,cream);
    float lift = m_Time > 8 ? std::sin(std::min((m_Time-8)*2,1.57f))*45 : 0;
    r.DrawLine({chefX+20,390+bob},{chefX+65,390-lift},10,cream);
    r.DrawLine({chefX+65,410-lift},{chefX+65,345-lift},6,glm::vec4(.6f,.67f,.69f,1));
    r.DrawQuad(chefX+50,320-lift,30,28,glm::vec4(.6f,.67f,.69f,1));
    float antX = 1200 - std::min(std::max(m_Time-2,0.0f)/6,1.0f)*410;
    if (m_Time > 10) antX += std::sin(m_Time*9)*6;
    for(int i=0;i<3;++i) {
        float x=antX+i*28;
        r.DrawCircle({x,458},18,glm::vec4(.10f,.07f,.05f,1));
        float step=std::sin(m_Time*10+i)*9;
        r.DrawLine({x,463},{x-15,490+step},4,cream);
        r.DrawLine({x,463},{x+15,490-step},4,cream);
    }
    r.DrawLine({antX-8,447},{antX-27,424},3,cream);
    r.DrawLine({antX+3,444},{antX-6,418},3,cream);
    r.DrawCircle({antX-9,453},3,cream);
    CenterText(r,"KITCHEN DEFENSE",48,3.5f,green);
    r.DrawQuad(0,550,1280,170,glm::vec4(.07f,.10f,.09f,1));
    const char* line = m_Time < 4 ? "CHEF: TOMORROW'S FEAST IS SAFE IN OUR FRIDGE."
        : m_Time < 8 ? "ANT: NOT FOR LONG! THAT FOOD WILL FEED OUR COLONY."
        : m_Time < 12 ? "CHEF: YOU WILL HAVE TO GET THROUGH US FIRST!"
        : "CHEF: DEFEND THE LANES. PROTECT THE FEAST!";
    CenterText(r,line,585,1.8f,cream);
    CenterText(r,"SPACE: SKIP INTRO     ESC: MAIN MENU",658,1.4f,green);
    r.DrawQuad(140,630,1000,5,glm::vec4(.24f,.30f,.25f,1));
    r.DrawQuad(140,630,1000*std::min(m_Time/15,1.0f),5,green);
}
// Drawn using the same vector renderer as the kitchen and characters.
void GameUI::RenderControls(Renderer2D& r) {
    const glm::vec4 ink(.23f,.19f,.13f,1);
    r.DrawQuad(40,145,290,445,glm::vec4(.10f,.08f,.04f,.5f));
    r.DrawQuad(32,137,290,445,glm::vec4(.92f,.83f,.62f,1));
    r.DrawRectOutline(42,147,270,425,2,glm::vec4(.65f,.48f,.26f,1));
    r.DrawQuad(135,127,85,22,glm::vec4(.64f,.66f,.59f,1));
    r.DrawText("CHEF'S CONTROLS",58,170,2.0f,ink);
    r.DrawText("YOUR KITCHEN FIELD GUIDE",58,200,1.3f,ink);
    struct Hint { const char* key; const char* action; };
    const Hint hints[] = {
        {"LEFT CLICK", "SELECT / UNSELECT LANES"},
        {"CLICK A CARD", "DEPLOY TO SELECTED LANES"},
        {"W / UP", "JUMP OVER LOW ATTACKS"},
        {"S / DOWN", "DUCK UNDER HIGH ATTACKS"},
        {"R", "RESTART THE GAME"},
        {"ESC", "PAUSE / CONTINUE"}
    };
    for (int i=0; i<6; ++i) {
        float y=239.0f+i*48;
        r.DrawQuad(56,y-5,242,20,glm::vec4(.82f,.71f,.49f,1));
        r.DrawText(hints[i].key,63,y,1.6f,ink);
        r.DrawText(hints[i].action,58,y+21,1.35f,ink,1.4f);
    }
    r.DrawText("SELECT LANES BEFORE A CARD",58,550,1.35f,ink);
}
void GameUI::RenderResultScene(Renderer2D& r, bool won) {
    const glm::vec4 accent = won ? green : glm::vec4(.98f,.49f,.33f,1);
    // Backsplash tiles and a wooden worktop frame the result chalkboard.
    r.DrawQuad(0,0,1280,720,won ? glm::vec4(.20f,.29f,.23f,1) : glm::vec4(.27f,.19f,.16f,1));
    for(int x=0;x<1280;x+=80) r.DrawLine({float(x),0},{float(x),560},2,glm::vec4(.36f,.37f,.28f,.5f));
    for(int y=0;y<560;y+=70) r.DrawLine({0,float(y)},{1280,float(y)},2,glm::vec4(.36f,.37f,.28f,.5f));
    r.DrawQuad(0,560,1280,160,glm::vec4(.48f,.30f,.16f,1));
    for(int y=580;y<720;y+=28) r.DrawLine({0,float(y)},{1280,float(y)},2,glm::vec4(.61f,.40f,.22f,1));
    // Fridge illustration: stocked and sealed on victory, raided on defeat.
    r.DrawQuad(100,241,200,310,glm::vec4(.10f,.12f,.11f,.5f));
    r.DrawQuad(88,229,200,310,glm::vec4(.69f,.75f,.70f,1));
    r.DrawRectOutline(98,239,180,92,3,cream);
    r.DrawRectOutline(98,342,180,187,3,cream);
    r.DrawQuad(118,362,140,123,glm::vec4(.16f,.22f,.17f,1));
    r.DrawQuad(261,265,6,45,cream); r.DrawQuad(261,375,6,60,cream);
    if(won) {
        for(int i=0;i<3;++i) {
            r.DrawQuad(127+i*41,417,30,36,glm::vec4(.86f,.56f,.22f,1));
            r.DrawCircle({142.0f+i*41,417},15,cream);
        }
        // Chef hat and small celebratory sparkles above the protected fridge.
        r.DrawQuad(147,201,80,24,cream);
        for(int i=0;i<3;++i) r.DrawCircle({159.0f+i*23,192},22,cream);
        for(int i=0;i<9;++i) {
            float x=65.0f+(i%3)*105, y=100.0f+(i/3)*35+std::sin(m_Time*2+i)*5;
            r.DrawLine({x-4,y},{x+4,y},2,accent);
            r.DrawLine({x,y-4},{x,y+4},2,accent);
        }
    } else {
        // An open door and an ant carrying away the last crumb.
        r.DrawQuadVertices({278,342},{320,363},{320,541},{278,529},glm::vec4(.51f,.59f,.53f,1));
        for(int i=0;i<3;++i) {
            float x=145.0f+i*22, y=462+std::sin(m_Time*5)*2;
            r.DrawCircle({x,y},13,glm::vec4(.08f,.06f,.04f,1));
            r.DrawLine({x,y+5},{x-10,y+24},3,cream);
            r.DrawLine({x,y+5},{x+10,y+24},3,cream);
        }
        r.DrawLine({141,451},{125,435},3,cream);
        r.DrawLine({149,451},{145,431},3,cream);
        r.DrawQuad(115,420,20,16,glm::vec4(.90f,.64f,.28f,1));
    }
    r.DrawText(won ? "FRIDGE SECURED" : "EMPTY SHELVES!",90,573,1.8f,cream);
    // A kitchen order ticket gives each outcome its own story.
    r.DrawQuad(965,235,265,230,glm::vec4(.96f,.88f,.69f,1));
    const glm::vec4 ink(.28f,.21f,.13f,1);
    r.DrawText("KITCHEN REPORT",985,260,2.0f,ink);
    r.DrawLine({985,289},{1210,289},2,ink);
    r.DrawText(won ? "FEAST: READY TO SERVE" : "FEAST: STOLEN",985,312,1.5f,ink);
    r.DrawText(won ? "PESTS: ALL CLEARED" : "PESTS: IN THE FRIDGE",985,347,1.5f,ink);
    r.DrawText(won ? "CHEF: EXCELLENT WORK!" : "CHEF: TRY AGAIN!",985,382,1.5f,ink);
    r.DrawText(won ? "SERVICE SAVED" : "KITCHEN CLOSED",985,430,1.9f,won ? glm::vec4(.20f,.42f,.19f,1) : glm::vec4(.65f,.20f,.12f,1));
}
void GameUI::Render(Renderer2D& r) {
    if (m_Screen == Screen::Playing) m_Field.Render(r);
    else if (m_Screen == Screen::Intro) RenderIntro(r);
    else {
        if (m_Screen == Screen::Paused) { m_Field.Render(r); r.DrawQuad(0,0,1280,720,glm::vec4(.02f,.06f,.04f,.87f)); }
        else if (m_Screen == Screen::Won || m_Screen == Screen::Lost)
            RenderResultScene(r, m_Screen == Screen::Won);
        else r.DrawQuad(0,0,1280,720,glm::vec4(.08f,.15f,.12f,1));
        const bool result = m_Screen == Screen::Won || m_Screen == Screen::Lost;
        const glm::vec4 accent = m_Screen == Screen::Lost ? glm::vec4(.98f,.49f,.33f,1) : green;
        if (result) {
            r.DrawQuad(358,98,564,534,glm::vec4(.49f,.31f,.16f,1));
            r.DrawQuad(370,110,540,510,glm::vec4(.08f,.14f,.11f,1));
        }
        if (m_Screen == Screen::Paused) RenderControls(r);
        r.DrawRectOutline(370,110,540,510,3,glm::vec4(.43f,.57f,.37f,1));
        const std::string title = m_Screen == Screen::Main ? "KITCHEN DEFENSE" : m_Screen == Screen::Paused ? "PAUSED" : m_Screen == Screen::Won ? "FEAST SAVED!" : "FRIDGE INVADED!";
        CenterText(r,title,170,3.2f,accent);
        if (result) CenterText(r,m_Screen == Screen::Won ? "THE PESTS ARE GONE. DINNER IS SAFE!" : "THE PESTS GOT TO THE FEAST FIRST.",215,1.5f,cream);
        const std::string subtitle = m_Screen == Screen::Main ? "PROTECT THE FRIDGE FROM THE PESTS" : m_Screen == Screen::Paused ? "TAKE A BREATHER, CHEF" : "FINAL SCORE: " + std::to_string(m_Field.GetGameLogic().GetScore());
        CenterText(r,subtitle,245,1.6f,cream);
        const char* labels[3] = {"RESTART", "MAIN MENU", "MAIN MENU"};
        if (m_Screen == Screen::Main) { labels[0]="START GAME"; labels[1]="EXIT"; }
        if (m_Screen == Screen::Paused) { labels[0]="CONTINUE"; labels[1]="RESTART"; }
        if (result) labels[0] = m_Screen == Screen::Won ? "ANOTHER SERVICE" : "TAKE BACK THE KITCHEN";
        for (int i=0;i<ButtonCount();++i) {
            bool selected = i == m_Focused;
            r.DrawQuad(430,330+i*76,420,58,selected ? glm::vec4(.29f,.44f,.25f,1) : glm::vec4(.15f,.24f,.19f,1));
            r.DrawRectOutline(430,330+i*76,420,58,2,selected ? accent : glm::vec4(.32f,.42f,.32f,1));
            CenterText(r,labels[i],350+i*76,1.9f,cream);
        }
        CenterText(r,"MOUSE OR ARROW KEYS + ENTER",650,1.4f,cream);
        CenterText(r,m_Screen == Screen::Paused ? "GAMEPLAY IS FROZEN | ESC TO CONTINUE" : "SELECT LANES THEN CLICK A CARD | ESC DURING PLAY FOR CONTROLS",688,1.3f,accent);
    }
    if (m_Fade > 0) r.DrawQuad(0,0,1280,720,glm::vec4(0,0,0,m_Fade/.25f));
}
