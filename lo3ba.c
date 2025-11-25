#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <random>

class Game {
private:
    sf::RenderWindow window;
    sf::View gameView;
    sf::View uiView;
    
    // Player
    sf::RectangleShape player;
    sf::Vector2f playerVelocity;
    bool onGround;
    float playerSpeed;
    int health;
    
    // Granny
    sf::RectangleShape granny;
    sf::Texture grannyTexture;
    sf::Vector2f grannyVelocity;
    float grannySpeed;
    enum class GrannyState { PATROL, CHASE, SEARCH };
    GrannyState grannyState;
    float awareness;
    sf::Vector2f lastSeenPosition;
    float searchTimer;
    
    // Map
    std::vector<sf::RectangleShape> walls;
    std::vector<sf::RectangleShape> doors;
    std::vector<sf::RectangleShape> hidingSpots;
    
    // Items
    struct Item {
        sf::RectangleShape shape;
        std::string type;
        bool collected;
        sf::Texture texture;
    };
    std::vector<Item> items;
    
    // Game state
    int day;
    float time;
    bool gameOver;
    bool gameWon;
    bool mapVisible;
    
    // UI
    sf::Font font;
    sf::Text dayText;
    sf::RectangleShape healthBar;
    sf::RectangleShape healthBarBackground;
    sf::RectangleShape miniMap;
    std::vector<sf::RectangleShape> inventorySlots;
    
    // Input
    std::map<sf::Keyboard::Key, bool> keys;
    
    // Sounds
    sf::SoundBuffer jumpScareBuffer;
    sf::Sound jumpScareSound;
    
public:
    Game() : window(sf::VideoMode(1200, 800), "3D-Style Granny Horror Game", sf::Style::Close),
             playerVelocity(0, 0), onGround(false), playerSpeed(300.0f),
             grannyVelocity(0, 0), grannySpeed(150.0f), grannyState(GrannyState::PATROL),
             awareness(0), searchTimer(0), day(1), time(7.0f), gameOver(false),
             gameWon(false), mapVisible(false) {
        
        window.setFramerateLimit(60);
        
        // Setup views
        gameView.setSize(1200, 800);
        gameView.setCenter(600, 400);
        
        uiView.setSize(1200, 800);
        uiView.setCenter(600, 400);
        
        initializeGame();
    }
    
    void initializeGame() {
        // Initialize player
        player.setSize(sf::Vector2f(30, 50));
        player.setFillColor(sf::Color::Green);
        player.setPosition(100, 100);
        health = 100;
        
        // Initialize Granny
        granny.setSize(sf::Vector2f(40, 60));
        granny.setFillColor(sf::Color::Magenta);
        granny.setPosition(800, 500);
        
        // Load Granny texture (creepy face)
        if (!grannyTexture.loadFromFile("granny_face.png")) {
            // Create a simple creepy face pattern
            sf::Image faceImage;
            faceImage.create(40, 60, sf::Color::Magenta);
            
            // Draw creepy eyes
            for (int i = 10; i < 15; i++) {
                for (int j = 15; j < 20; j++) {
                    faceImage.setPixel(i, j, sf::Color::Red);
                    faceImage.setPixel(i + 15, j, sf::Color::Red);
                }
            }
            
            // Draw creepy mouth
            for (int i = 12; i < 28; i++) {
                for (int j = 35; j < 38; j++) {
                    faceImage.setPixel(i, j, sf::Color::Black);
                }
            }
            
            grannyTexture.loadFromImage(faceImage);
        }
        granny.setTexture(&grannyTexture);
        
        createMap();
        createItems();
        setupUI();
        loadSounds();
    }
    
    void createMap() {
        // Create walls for rooms
        // Bedroom
        createRoom(50, 50, 300, 250);
        // Hallway
        createRoom(400, 50, 150, 500);
        // Living Room
        createRoom(600, 50, 350, 250);
        // Kitchen
        createRoom(600, 350, 350, 250);
        // Bathroom
        createRoom(50, 350, 300, 200);
        // Storage
        createRoom(50, 600, 300, 150);
        
        // Create doors (openings)
        sf::RectangleShape door;
        door.setSize(sf::Vector2f(20, 60));
        door.setFillColor(sf::Color::Transparent);
        
        // Bedroom to Hallway
        door.setPosition(350, 120);
        doors.push_back(door);
        
        // Hallway to Living Room
        door.setPosition(550, 120);
        doors.push_back(door);
        
        // Hallway to Kitchen
        door.setPosition(550, 320);
        doors.push_back(door);
        
        // Hallway to Bathroom
        door.setPosition(350, 320);
        doors.push_back(door);
        
        // Create hiding spots (closets)
        sf::RectangleShape closet;
        closet.setSize(sf::Vector2f(80, 100));
        closet.setFillColor(sf::Color(139, 69, 19)); // Brown
        
        closet.setPosition(80, 80);
        hidingSpots.push_back(closet);
        
        closet.setPosition(650, 80);
        hidingSpots.push_back(closet);
        
        closet.setPosition(80, 620);
        hidingSpots.push_back(closet);
    }
    
    void createRoom(float x, float y, float width, float height) {
        // Top wall
        walls.push_back(createWall(x, y, width, 20));
        // Bottom wall
        walls.push_back(createWall(x, y + height - 20, width, 20));
        // Left wall
        walls.push_back(createWall(x, y, 20, height));
        // Right wall
        walls.push_back(createWall(x + width - 20, y, 20, height));
    }
    
    sf::RectangleShape createWall(float x, float y, float width, float height) {
        sf::RectangleShape wall;
        wall.setSize(sf::Vector2f(width, height));
        wall.setPosition(x, y);
        wall.setFillColor(sf::Color(100, 100, 100));
        return wall;
    }
    
    void createItems() {
        std::vector<std::pair<std::string, sf::Vector2f>> itemData = {
            {"key", sf::Vector2f(150, 150)},
            {"hammer", sf::Vector2f(750, 150)},
            {"screwdriver", sf::Vector2f(750, 450)},
            {"battery", sf::Vector2f(150, 400)},
            {"master_key", sf::Vector2f(150, 650)}
        };
        
        for (const auto& data : itemData) {
            Item item;
            item.shape.setSize(sf::Vector2f(20, 20));
            item.shape.setPosition(data.second);
            item.type = data.first;
            item.collected = false;
            
            // Color code items
            if (data.first == "key") item.shape.setFillColor(sf::Color::Yellow);
            else if (data.first == "hammer") item.shape.setFillColor(sf::Color(165, 42, 42)); // Brown
            else if (data.first == "screwdriver") item.shape.setFillColor(sf::Color::Blue);
            else if (data.first == "battery") item.shape.setFillColor(sf::Color::Green);
            else if (data.first == "master_key") item.shape.setFillColor(sf::Color::Cyan);
            
            items.push_back(item);
        }
    }
    
    void setupUI() {
        // Load font
        if (!font.loadFromFile("arial.ttf")) {
            std::cerr << "Failed to load font, using default\n";
        }
        
        // Day text
        dayText.setFont(font);
        dayText.setCharacterSize(20);
        dayText.setFillColor(sf::Color::White);
        dayText.setPosition(20, 20);
        
        // Health bar
        healthBarBackground.setSize(sf::Vector2f(200, 20));
        healthBarBackground.setPosition(20, 50);
        healthBarBackground.setFillColor(sf::Color(50, 50, 50));
        
        healthBar.setSize(sf::Vector2f(200, 20));
        healthBar.setPosition(20, 50);
        healthBar.setFillColor(sf::Color::Red);
        
        // Mini-map
        miniMap.setSize(sf::Vector2f(200, 200));
        miniMap.setPosition(980, 20);
        miniMap.setFillColor(sf::Color(0, 0, 0, 150));
        miniMap.setOutlineThickness(2);
        miniMap.setOutlineColor(sf::Color::White);
        
        // Inventory slots
        for (int i = 0; i < 5; i++) {
            sf::RectangleShape slot;
            slot.setSize(sf::Vector2f(40, 40));
            slot.setPosition(980 + i * 50, 750);
            slot.setFillColor(sf::Color(50, 50, 50));
            slot.setOutlineThickness(2);
            slot.setOutlineColor(sf::Color::White);
            inventorySlots.push_back(slot);
        }
    }
    
    void loadSounds() {
        // In a real implementation, you'd load actual sound files
        // For now, we'll create placeholder sounds
        if (!jumpScareBuffer.loadFromFile("jumpscare.wav")) {
            std::cerr << "Failed to load jump scare sound\n";
        }
        jumpScareSound.setBuffer(jumpScareBuffer);
    }
    
    void run() {
        sf::Clock clock;
        
        while (window.isOpen()) {
            sf::Time deltaTime = clock.restart();
            float dt = deltaTime.asSeconds();
            
            processEvents();
            if (!gameOver && !gameWon) {
                update(dt);
            }
            render();
        }
    }
    
    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                keys[event.key.code] = true;
                
                if (event.key.code == sf::Keyboard::M) {
                    mapVisible = !mapVisible;
                }
                if (event.key.code == sf::Keyboard::R && (gameOver || gameWon)) {
                    resetGame();
                }
            }
            
            if (event.type == sf::Event::KeyReleased) {
                keys[event.key.code] = false;
            }
        }
    }
    
    void update(float dt) {
        updatePlayer(dt);
        updateGranny(dt);
        updateItems();
        updateTime(dt);
        checkCollisions();
        checkWinCondition();
        updateUI();
    }
    
    void updatePlayer(float dt) {
        // Handle input
        playerVelocity.x = 0;
        playerVelocity.y = 0;
        
        if (keys[sf::Keyboard::W]) playerVelocity.y = -playerSpeed;
        if (keys[sf::Keyboard::S]) playerVelocity.y = playerSpeed;
        if (keys[sf::Keyboard::A]) playerVelocity.x = -playerSpeed;
        if (keys[sf::Keyboard::D]) playerVelocity.x = playerSpeed;
        
        // Normalize diagonal movement
        if (playerVelocity.x != 0 && playerVelocity.y != 0) {
            playerVelocity.x *= 0.707f;
            playerVelocity.y *= 0.707f;
        }
        
        // Apply movement
        sf::Vector2f newPosition = player.getPosition() + playerVelocity * dt;
        
        // Check wall collisions
        if (!checkWallCollision(newPosition)) {
            player.setPosition(newPosition);
        }
        
        // Keep player in bounds
        sf::Vector2f pos = player.getPosition();
        pos.x = std::max(0.0f, std::min(1150.0f, pos.x));
        pos.y = std::max(0.0f, std::min(750.0f, pos.y));
        player.setPosition(pos);
        
        // Update camera to follow player
        gameView.setCenter(player.getPosition());
    }
    
    void updateGranny(float dt) {
        sf::Vector2f playerPos = player.getPosition();
        sf::Vector2f grannyPos = granny.getPosition();
        
        // Calculate distance to player
        float dx = playerPos.x - grannyPos.x;
        float dy = playerPos.y - grannyPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // Update Granny's state
        if (distance < 200 && hasLineOfSight()) {
            grannyState = GrannyState::CHASE;
            awareness = 100;
            lastSeenPosition = playerPos;
            searchTimer = 3.0f;
        } else if (grannyState == GrannyState::CHASE) {
            grannyState = GrannyState::SEARCH;
            searchTimer = 3.0f;
        } else if (grannyState == GrannyState::SEARCH && searchTimer > 0) {
            searchTimer -= dt;
        } else {
            grannyState = GrannyState::PATROL;
            awareness = std::max(0.0f, awareness - 50.0f * dt);
        }
        
        // Move Granny based on state
        switch (grannyState) {
            case GrannyState::PATROL:
                patrolBehavior(dt);
                break;
            case GrannyState::CHASE:
                chaseBehavior(dt);
                break;
            case GrannyState::SEARCH:
                searchBehavior(dt);
                break;
        }
        
        // Check if caught player
        if (distance < 50) {
            playerCaught();
        }
    }
    
    void patrolBehavior(float dt) {
        static sf::Vector2f targetPosition = granny.getPosition();
        static float changeTargetTimer = 0;
        
        changeTargetTimer -= dt;
        if (changeTargetTimer <= 0 || 
            std::abs(granny.getPosition().x - targetPosition.x) < 10 &&
            std::abs(granny.getPosition().y - targetPosition.y) < 10) {
            
            // New random target
            targetPosition.x = static_cast<float>(rand() % 1100 + 50);
            targetPosition.y = static_cast<float>(rand() % 700 + 50);
            changeTargetTimer = 5.0f;
        }
        
        // Move towards target
        sf::Vector2f direction = targetPosition - granny.getPosition();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction /= length;
            granny.move(direction * grannySpeed * dt);
        }
    }
    
    void chaseBehavior(float dt) {
        sf::Vector2f direction = player.getPosition() - granny.getPosition();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction /= length;
            granny.move(direction * grannySpeed * 1.5f * dt);
        }
    }
    
    void searchBehavior(float dt) {
        sf::Vector2f direction = lastSeenPosition - granny.getPosition();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0) {
            direction /= length;
            granny.move(direction * grannySpeed * dt);
        }
        
        // Random wandering while searching
        if (rand() % 100 < 5) {
            lastSeenPosition.x += static_cast<float>(rand() % 100 - 50);
            lastSeenPosition.y += static_cast<float>(rand() % 100 - 50);
        }
    }
    
    bool hasLineOfSight() {
        // Simple line of sight check
        // In a real implementation, you'd do proper raycasting
        sf::Vector2f playerPos = player.getPosition();
        sf::Vector2f grannyPos = granny.getPosition();
        
        // Check if there are walls between them
        for (const auto& wall : walls) {
            if (lineIntersectsRect(playerPos, grannyPos, wall.getGlobalBounds())) {
                return false;
            }
        }
        
        return true;
    }
    
    bool lineIntersectsRect(sf::Vector2f p1, sf::Vector2f p2, sf::FloatRect rect) {
        // Check if line intersects with rectangle
        return lineIntersectsLine(p1, p2, sf::Vector2f(rect.left, rect.top), sf::Vector2f(rect.left + rect.width, rect.top)) ||
               lineIntersectsLine(p1, p2, sf::Vector2f(rect.left + rect.width, rect.top), sf::Vector2f(rect.left + rect.width, rect.top + rect.height)) ||
               lineIntersectsLine(p1, p2, sf::Vector2f(rect.left, rect.top + rect.height), sf::Vector2f(rect.left + rect.width, rect.top + rect.height)) ||
               lineIntersectsLine(p1, p2, sf::Vector2f(rect.left, rect.top), sf::Vector2f(rect.left, rect.top + rect.height));
    }
    
    bool lineIntersectsLine(sf::Vector2f l1p1, sf::Vector2f l1p2, sf::Vector2f l2p1, sf::Vector2f l2p2) {
        // Implementation of line-line intersection
        float den = (l1p1.x - l1p2.x) * (l2p1.y - l2p2.y) - (l1p1.y - l1p2.y) * (l2p1.x - l2p2.x);
        if (den == 0) return false;
        
        float t = ((l1p1.x - l2p1.x) * (l2p1.y - l2p2.y) - (l1p1.y - l2p1.y) * (l2p1.x - l2p2.x)) / den;
        float u = -((l1p1.x - l1p2.x) * (l1p1.y - l2p1.y) - (l1p1.y - l1p2.y) * (l1p1.x - l2p1.x)) / den;
        
        return t >= 0 && t <= 1 && u >= 0 && u <= 1;
    }
    
    void updateItems() {
        for (auto& item : items) {
            if (!item.collected) {
                sf::FloatRect itemBounds = item.shape.getGlobalBounds();
                sf::FloatRect playerBounds = player.getGlobalBounds();
                
                if (itemBounds.intersects(playerBounds)) {
                    item.collected = true;
                    // In a full implementation, add to inventory
                }
            }
        }
    }
    
    void updateTime(float dt) {
        time += dt * 0.1f;
        if (time >= 24.0f) {
            time = 7.0f;
            day++;
            
            if (day > 5) {
                gameOver = true;
            }
        }
    }
    
    void checkCollisions() {
        // Check wall collisions (already handled in updatePlayer)
    }
    
    void checkWinCondition() {
        // Check if player has all items and reached exit
        bool hasAllItems = true;
        for (const auto& item : items) {
            if (!item.collected) {
                hasAllItems = false;
                break;
            }
        }
        
        sf::Vector2f playerPos = player.getPosition();
        if (hasAllItems && playerPos.x > 1000 && playerPos.y < 100) {
            gameWon = true;
        }
    }
    
    void playerCaught() {
        health -= 25;
        jumpScareSound.play();
        
        // Reset positions
        player.setPosition(100, 100);
        granny.setPosition(800, 500);
        
        if (health <= 0) {
            gameOver = true;
        }
    }
    
    bool checkWallCollision(sf::Vector2f newPosition) {
        sf::FloatRect newBounds(newPosition.x, newPosition.y, player.getSize().x, player.getSize().y);
        
        for (const auto& wall : walls) {
            if (newBounds.intersects(wall.getGlobalBounds())) {
                return true;
            }
        }
        
        return false;
    }
    
    void updateUI() {
        // Update day text
        int hours = static_cast<int>(time);
        int minutes = static_cast<int>((time - hours) * 60);
        std::string ampm = hours >= 12 ? "PM" : "AM";
        int displayHours = hours % 12;
        if (displayHours == 0) displayHours = 12;
        
        dayText.setString("Day " + std::to_string(day) + " - " + 
                         std::to_string(displayHours) + ":" + 
                         (minutes < 10 ? "0" : "") + std::to_string(minutes) + " " + ampm);
        
        // Update health bar
        healthBar.setSize(sf::Vector2f(health * 2, 20));
    }
    
    void render() {
        window.clear(sf::Color(20, 20, 40)); // Dark blue background
        
        // Draw game world
        window.setView(gameView);
        
        // Draw rooms and walls
        for (const auto& wall : walls) {
            window.draw(wall);
        }
        
        // Draw doors
        for (const auto& door : doors) {
            window.draw(door);
        }
        
        // Draw hiding spots
        for (const auto& closet : hidingSpots) {
            window.draw(closet);
        }
        
        // Draw items
        for (const auto& item : items) {
            if (!item.collected) {
                window.draw(item.shape);
            }
        }
        
        // Draw Granny
        window.draw(granny);
        
        // Draw player
        window.draw(player);
        
        // Draw UI
        window.setView(uiView);
        
        window.draw(healthBarBackground);
        window.draw(healthBar);
        window.draw(dayText);
        
        if (mapVisible) {
            window.draw(miniMap);
            drawMiniMap();
        }
        
        // Draw inventory
        for (const auto& slot : inventorySlots) {
            window.draw(slot);
        }
        
        // Draw game over or win screen
        if (gameOver) {
            drawGameOverScreen();
        } else if (gameWon) {
            drawWinScreen();
        }
        
        window.display();
    }
    
    void drawMiniMap() {
        // In a full implementation, draw mini-map representation
        sf::RectangleShape playerMini(sf::Vector2f(6, 6));
        playerMini.setFillColor(sf::Color::Green);
        playerMini.setPosition(miniMap.getPosition() + sf::Vector2f(
            (player.getPosition().x / 1200.0f) * 180 + 10,
            (player.getPosition().y / 800.0f) * 180 + 10
        ));
        window.draw(playerMini);
        
        sf::RectangleShape grannyMini(sf::Vector2f(6, 6));
        grannyMini.setFillColor(sf::Color::Magenta);
        grannyMini.setPosition(miniMap.getPosition() + sf::Vector2f(
            (granny.getPosition().x / 1200.0f) * 180 + 10,
            (granny.getPosition().y / 800.0f) * 180 + 10
        ));
        window.draw(grannyMini);
    }
    
    void drawGameOverScreen() {
        sf::RectangleShape overlay(sf::Vector2f(1200, 800));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);
        
        sf::Text gameOverText;
        gameOverText.setFont(font);
        gameOverText.setString("GAME OVER\nGranny caught you!\nPress R to restart");
        gameOverText.setCharacterSize(48);
        gameOverText.setFillColor(sf::Color::Red);
        gameOverText.setPosition(400, 300);
        gameOverText.setStyle(sf::Text::Bold);
        window.draw(gameOverText);
    }
    
    void drawWinScreen() {
        sf::RectangleShape overlay(sf::Vector2f(1200, 800));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);
        
        sf::Text winText;
        winText.setFont(font);
        winText.setString("YOU ESCAPED!\nYou survived Granny's house!\nPress R to play again");
        winText.setCharacterSize(48);
        winText.setFillColor(sf::Color::Green);
        winText.setPosition(350, 300);
        winText.setStyle(sf::Text::Bold);
        window.draw(winText);
    }
    
    void resetGame() {
        // Reset game state
        player.setPosition(100, 100);
        granny.setPosition(800, 500);
        health = 100;
        day = 1;
        time = 7.0f;
        gameOver = false;
        gameWon = false;
        awareness = 0;
        grannyState = GrannyState::PATROL;
        
        // Reset items
        for (auto& item : items) {
            item.collected = false;
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
