#include "chip8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <string>


#include "./imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"


const int SCALE = 10; // Each pixel is 10x10 screen pixels
const int WIDTH = 64*SCALE;
const int HEIGHT = 32*SCALE;

// Keyboard mapping
uint8_t keymap[16] = {
    SDLK_x, // 0
    SDLK_1, // 1
    SDLK_2, // 2
    SDLK_3, // 3
    SDLK_q, // 4
    SDLK_w, // 5
    SDLK_e, // 6
    SDLK_a, // 7
    SDLK_s, // 8
    SDLK_d, // 9
    SDLK_z, // A
    SDLK_c, // B
    SDLK_4, // C
    SDLK_r, // D
    SDLK_f, // E
    SDLK_v  // F
};

int color_filter = 0;
int emulation_speed = 10;
int screen = 0;
bool pause_menu = false;
std::string popup_message = "";
Uint32 popup_until = 0;
std::string current_rom;
bool resizable_window = false;

void audio_callback(void* userdata, uint8_t* stream, int len){
    static uint32_t sample_index = 0;
    int16_t* audio_buffer = (int16_t*) stream;
    int samples = len/2;

    bool* beeping = (bool*) userdata;
    for(int i=0; i<samples; i++){
        if(*beeping){
            // Generating 440Hz sqaure wave
            int16_t value = ((sample_index++ / 50) % 2) ? 3000 : -3000;
            audio_buffer[i] = value;
        }
        else{
            audio_buffer[i] = 0; // Silence
            sample_index = 0;
        }
    }
}

void draw_graphics(SDL_Renderer* renderer, Chip8& chip8){
    // Clear screen
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    switch(color_filter){
    	case 0:
    		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    		break;
	case 1:
    		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    		break;
    	case 2:
    		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    		break;
    	case 3:
    		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    		break;
    	case 4:
    		SDL_SetRenderDrawColor(renderer,18,9,26,255);
    		break;
    	
    }
    SDL_RenderClear(renderer);
    // Drawing white pixels
    switch(color_filter){
    	case 0:
    		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    		break;
	case 1:
    		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    		break;
    	case 2:
    		SDL_SetRenderDrawColor(renderer, 255, 176, 0, 255);
    		break;
    	case 3:
    		SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    		break;
    	case 4:
    		SDL_SetRenderDrawColor(renderer,214,107,255,255);
    		break;
    }
    
    int window_width;
    int window_height;
    
    SDL_GetRendererOutputSize(renderer, &window_width, &window_height);
    
    int pixel_width = SCALE;
    int pixel_height = SCALE;
    
    if(resizable_window){
    	pixel_width = window_width / 64;
    	pixel_height = window_height / 32;
    }
    
    
    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            if(chip8.display[x + (y*64)] == 1){
                SDL_Rect rect = {x*pixel_width, y*pixel_height, pixel_width, pixel_height};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}


void handle_input(Chip8& chip8, bool& running){
    SDL_Event event;

    while(SDL_PollEvent(&event)){
        ImGui_ImplSDL2_ProcessEvent(&event);

        if(event.type == SDL_QUIT) running = false;
        
        if(event.type == SDL_KEYUP){
            for(int i=0; i<16; i++){
                if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 0;
            }
        }
        
        
        if(event.type == SDL_KEYDOWN){
            for(int i=0; i<16; i++){
                if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 1;
            }
        }
        
        
        if(event.type == SDL_KEYDOWN){
            
            
            if(event.key.keysym.sym == SDLK_ESCAPE){
                if(screen == 1)
                    pause_menu = !pause_menu;
                else
                    running = false;
            }
            // Check which Chip-8 key was pressed
        
        if(event.key.keysym.sym == SDLK_F2){
            color_filter = 0;
            popup_message = "Color: Original";
            popup_until = SDL_GetTicks() + 1000;
        }

        if(event.key.keysym.sym == SDLK_F3){
            color_filter = 1;
            popup_message = "Color: Classic Green Screen";
            popup_until = SDL_GetTicks() + 1000;
        }

        if(event.key.keysym.sym == SDLK_F4){
            color_filter = 2;
            popup_message = "Color: Amber CRT";
            popup_until = SDL_GetTicks() + 1000;
        }

        if(event.key.keysym.sym == SDLK_F5){
            color_filter = 3;
            popup_message = "Color: Neon High-Contrast";
            popup_until = SDL_GetTicks() + 1000;
        }
        
        if(event.key.keysym.sym == SDLK_F8){
            emulation_speed--;
            popup_message = "Emulation Speed: " + std::to_string(emulation_speed);
            popup_until = SDL_GetTicks() + 1000;
        }
        
        if(event.key.keysym.sym == SDLK_F9){
            emulation_speed++;
            popup_message = "Emulation Speed: " + std::to_string(emulation_speed);
            popup_until = SDL_GetTicks() + 1000;
        }
            
        if(event.key.keysym.sym == SDLK_F10){
            chip8.save_state("save.state");
            popup_message = "State Saved";
            popup_until = SDL_GetTicks() + 1000;
        }
        
        if(event.key.keysym.sym == SDLK_F11){
            chip8.load_state("save.state");
            popup_message = "State Loaded";
            popup_until = SDL_GetTicks() + 1000;
        }
        }
    }
}

int main(int argc, char** argv){
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0){
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Audio setup
    bool beeping = false;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;
    want.userdata = &beeping;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if(audio_device == 0) std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    else SDL_PauseAudioDevice(audio_device, 0);

    SDL_Window* window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if(!window){
        std::cerr << "Window error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer){
        std::cerr << "Renderer error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

    Chip8 chip8;

    if(argc > 1){
        chip8.load_rom(argv[1]);
        screen = 1;
    }
    
    bool running = true;
    while(running){
        handle_input(chip8, running);

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if(screen == 0){
            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
            SDL_RenderClear(renderer);

            ImGui::SetNextWindowPos(
                ImVec2(WIDTH / 2 - 200, HEIGHT / 2 - 100),
                ImGuiCond_Always
            );

            ImGui::SetNextWindowSize(
                ImVec2(400, 200),
                ImGuiCond_Always
            );

            ImGui::Begin(
                "CHIP-8 Emulator",
                nullptr,
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse
            );

            ImGui::Text("Select a ROM");
            ImGui::Separator();

            if(std::filesystem::exists("roms")){
                for(const auto& entry : std::filesystem::directory_iterator("roms")){
                    if(entry.is_regular_file() && entry.path().extension() == ".ch8"){
                        std::string filename = entry.path().filename().string();

                        if(ImGui::Button(filename.c_str(), ImVec2(-1, 0))){
                            chip8 = Chip8();
                            chip8.load_rom(entry.path().string());
                            current_rom = entry.path().string();
                            screen = 1;
                        }
                    }
                }
            }
            else{
                ImGui::Text("No roms directory found.");
            }

            ImGui::Separator();
            ImGui::Text("ESC - Exit");

            ImGui::End();
        }
	
	if(screen == 1){
	if(!pause_menu){
    for(int i=0; i<emulation_speed; i++){
        chip8.emulate_cycle();
    }
}
    //SDL_Delay(16); // 60 FPS with 16ms per frame
    beeping = (chip8.get_sound_timer() > 0);
    draw_graphics(renderer, chip8);

    if(pause_menu){
        ImGui::SetNextWindowPos(
            //ImVec2(10, 10),
            ImVec2(WIDTH / 2.0f, HEIGHT / 2.0f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.5f)
        );

        ImGui::SetNextWindowBgAlpha(0.85f);

        ImGui::Begin(
            "CHIP-8",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoCollapse
        );
        
        ImGui::Text("Emulator Settings");
        
        ImGui::Separator();

        ImGui::Text("Emulation Speed: %d", emulation_speed);
        

        if(ImGui::Button("-"))
            emulation_speed--;

        ImGui::SameLine();

        if(ImGui::Button("+"))
            emulation_speed++;
	

	
		if(ImGui::Checkbox("Resizable Window", &resizable_window)){
        	if(resizable_window){
        		SDL_SetWindowResizable(window, SDL_TRUE);
        	} else {
        		SDL_SetWindowResizable(window, SDL_FALSE);
        		SDL_SetWindowSize(window, WIDTH, HEIGHT);
        		}
        }
	
        ImGui::Separator();
        
        ImGui::Text("Emulator State");
        
        ImGui::Separator();
        
        ImGui::Text("Save/Load Emulator State");
        
        if(ImGui::Button("Save State"))
        	chip8.save_state("save.state");

        ImGui::SameLine();

        if(ImGui::Button("Load State"))
        	chip8.load_state("save.state");
        
        ImGui::Text("Reset Emulation State");
		if(ImGui::Button("Reset")){
        	chip8 = Chip8();
        	chip8.load_rom(current_rom);
        	pause_menu = false;
        }
        
        ImGui::Separator();

        ImGui::Text("Color Settings");
        
        ImGui::Separator();
        
        ImGui::Text("Game Rendering Color");
	
	const char* items[] = {"Original", "Classic Green Screen", "Amber CRT", "Neon High-Contrast", "Purple Neon"};
	if(ImGui::Combo("##", &color_filter, items, IM_ARRAYSIZE(items))){}
        ImGui::Separator();
   
        if(ImGui::Button("Back to ROM Browser"))
            screen = 0;

        ImGui::End();
    }

    if(screen == 1 && SDL_GetTicks() < popup_until){
        ImGui::SetNextWindowPos(
            ImVec2(WIDTH / 2 - 100, HEIGHT - 60),
            ImGuiCond_Always
        );

        ImGui::SetNextWindowBgAlpha(0.85f);

        ImGui::Begin(
            "##popup",
            nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoInputs
        );

        ImGui::Text("%s", popup_message.c_str());

        ImGui::End();
    }
}

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if(audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

