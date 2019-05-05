#pragma once
#include "utility/geometry.h"
#include <SDL.h>
#include <memory>
struct Player_Song_Metadata;

class Metadata_Display {
public:
    explicit Metadata_Display(const Rect &bounds);
    ~Metadata_Display();

    void update_data(const Player_Song_Metadata &md);

    void paint(SDL_Renderer *rr);

private:
    const Rect bounds_;
    std::unique_ptr<Player_Song_Metadata> md_;
};
