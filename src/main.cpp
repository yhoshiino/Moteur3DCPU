#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

constexpr int WIDTH = 320;
constexpr int HEIGHT = 200;
constexpr int SCALE = 4;

// Format ARGB8888 : 0xAARRGGBB
constexpr uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b) {
	return (0xFFu << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

// Vertex en espace ecran : position en pixels + couleur (0..1)
struct Vertex2D {
	float x, y;
	float r, g, b;
};

// Fonction d'arete : positive d'un cote du segment a->b, negative de l'autre.
// Appliquee a (a, b, c), elle donne aussi 2x l'aire signee du triangle.
inline float edge(float ax, float ay, float bx, float by, float cx, float cy) {
	return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

void drawTriangle(std::vector<uint32_t>& fb, const Vertex2D& v0, const Vertex2D& v1, const Vertex2D& v2) {
	const float area = edge(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
	if (area == 0.0f) return; // triangle degenere

	// Boite englobante, limitee au framebuffer
	const int minX = std::max(0, (int)std::floor(std::min({ v0.x, v1.x, v2.x })));
	const int maxX = std::min(WIDTH - 1, (int)std::ceil(std::max({ v0.x, v1.x, v2.x })));
	const int minY = std::max(0, (int)std::floor(std::min({ v0.y, v1.y, v2.y })));
	const int maxY = std::min(HEIGHT - 1, (int)std::ceil(std::max({ v0.y, v1.y, v2.y })));

	for (int y = minY; y <= maxY; ++y) {
		for (int x = minX; x <= maxX; ++x) {
			// On teste le centre du pixel
			const float px = x + 0.5f;
			const float py = y + 0.5f;

			// Coordonnees barycentriques (divisees par l'aire : marche pour les deux sens d'enroulement)
			const float l0 = edge(v1.x, v1.y, v2.x, v2.y, px, py) / area;
			const float l1 = edge(v2.x, v2.y, v0.x, v0.y, px, py) / area;
			const float l2 = edge(v0.x, v0.y, v1.x, v1.y, px, py) / area;

			if (l0 < 0.0f || l1 < 0.0f || l2 < 0.0f) continue; // hors du triangle

			// Interpolation de la couleur
			const float r = l0 * v0.r + l1 * v1.r + l2 * v2.r;
			const float g = l0 * v0.g + l1 * v1.g + l2 * v2.g;
			const float b = l0 * v0.b + l1 * v1.b + l2 * v2.b;

			fb[y * WIDTH + x] = makeColor(
				(uint8_t)std::clamp(r * 255.0f, 0.0f, 255.0f),
				(uint8_t)std::clamp(g * 255.0f, 0.0f, 255.0f),
				(uint8_t)std::clamp(b * 255.0f, 0.0f, 255.0f));
		}
	}
}

int main() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << "SDL_Init a echoue : " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("Moteur3DCPU", WIDTH * SCALE, HEIGHT * SCALE, 0);
	if (!window) {
		std::cerr << "SDL_CreateWindow a echoue : " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
	if (!renderer) {
		std::cerr << "SDL_CreateRenderer a echoue : " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	if (!texture) {
		std::cerr << "SDL_CreateTexture a echoue : " << SDL_GetError() << std::endl;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

	std::vector<uint32_t> framebuffer(WIDTH * HEIGHT);


	const Vertex2D v0{ 160.0f,  30.0f, 1.0f, 0.0f, 0.0f };
	const Vertex2D v1{ 260.0f, 170.0f, 0.0f, 1.0f, 0.0f };
	const Vertex2D v2{ 60.0f, 170.0f, 0.0f, 0.0f, 1.0f };

	bool running = true;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
		}

		std::fill(framebuffer.begin(), framebuffer.end(), makeColor(20, 20, 30));
		drawTriangle(framebuffer, v0, v1, v2);

		SDL_UpdateTexture(texture, nullptr, framebuffer.data(), WIDTH * sizeof(uint32_t));
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}