# Realtime Game Server

A high-performance real-time multiplayer game server implementation in C++ with WebSocket support. Features server-side authoritative simulation, matchmaking, chat, and an interactive 8x8 grid-based game.

## Features

- **High-Performance Server**: C++ server running at 120 ticks/second with ultra-low latency network polling
- **Real-time Multiplayer**: WebSocket-based communication supporting multiple concurrent players
- **Interactive Game**: 8x8 grid-based game with player movement and shooting mechanics
- **Server-side Authoritative**: Ensures game integrity and prevents cheating
- **Matchmaking System**: Automatic player matching based on game mode and player count
- **Chat System**: Global and channel-based chat messaging
- **Client-side Prediction**: Instant local feedback for smooth player experience
- **Web Demo Client**: Beautiful dark-themed web interface for testing and playing
- **C# Client SDK**: Easy-to-use SDK for game client integration (Unity/Godot compatible)

## Building the Server

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- libwebsockets library (installed via Homebrew on macOS: `brew install libwebsockets`)
- jsoncpp library (installed via Homebrew on macOS: `brew install jsoncpp`)

### Build Steps

```bash
cd server
mkdir build
cd build
cmake ..
make
```

### Running the Server

```bash
cd server/build
./GameServer 8080
```

## Building the SDK

### Prerequisites

- .NET 6.0 SDK or later

### Build Steps

```bash
cd sdk
dotnet build
```

### Using the SDK

Add a reference to the SDK in your project:

```xml
<ProjectReference Include="path/to/GameServerSDK.csproj" />
```

Or install as a NuGet package (if published).

## Usage Example

### C# Client

```csharp
using GameServerSDK;

var sdk = new GameServerSDK("ws://localhost:8080");

sdk.OnMatchFound += (sender, e) => {
    Console.WriteLine($"Match found: {e.MatchId}");
};

await sdk.ConnectAsync();
await sdk.Matchmaking.QueueForMatchAsync("default", 2, 4);
await sdk.Chat.SendMessageAsync("Hello!");
await sdk.GameState.SendMoveAsync(10.0, 20.0, 5.0);
```

## Demo & Testing

### Web-Based Visual Demo

The easiest way to test the server is using the included web client:

1. Start the server (see "Running the Server" above)
2. Open `demo/web-client/index.html` in your browser
3. Click "Connect" to connect to the server
4. Click "Join Game Grid" to spawn on the 8x8 grid
5. Use arrow keys to move, Shift + arrow keys to shoot

The web client provides a minimalistic dark-themed interface with:
- Connection status and player stats
- Matchmaking controls
- Interactive 8x8 game board with client-side prediction
- Chat system with activity log
- Real-time game state synchronization

## Architecture

### Server-Side Authoritative Simulation

The server maintains the authoritative game state and validates all player actions. This prevents cheating by ensuring:
- Actions are validated before being applied
- Game state is always consistent across all clients
- State updates are broadcast only when changes occur (dirty state tracking)

### Client-Side Prediction

The web client implements client-side prediction for instant local feedback:
- Players see their own moves immediately (0ms latency)
- Server confirms and corrects if needed
- Smooth gameplay experience even with network latency

### Matchmaking

Players are queued by game mode and matched when enough players are available. The system:
- Groups players by game mode
- Matches players based on min/max player requirements
- Creates match instances and notifies all players

### Chat System

Supports multiple channels (global, match-specific, etc.):
- Real-time message broadcasting
- Activity log integration

## Performance

- **Tick Rate**: 120 ticks per second for ultra-low latency
- **Network Polling**: 1ms timeout for minimal delay
- **State Updates**: Only broadcasted when game state changes (dirty tracking)
- **Client Prediction**: Instant local feedback with server reconciliation
- **Optimized Rendering**: DOM recycling and efficient updates in web client

## License

This project is provided as-is for educational and portfolio purposes.

## Tech Stack

- **Server**: C++17 with libwebsockets
- **Web Client**: Vanilla JavaScript, HTML5, CSS3
- **Client SDK**: C# / .NET 6.0
- **Communication**: WebSockets
- **Serialization**: JSON (jsoncpp for C++)

