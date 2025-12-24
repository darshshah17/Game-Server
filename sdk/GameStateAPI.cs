using System;
using System.Threading.Tasks;

namespace GameServerSDK
{
    /// <summary>
    /// High-level API for game state synchronization
    /// </summary>
    public class GameStateAPI
    {
        private readonly GameServerClient _client;

        public GameStateAPI(GameServerClient client)
        {
            _client = client;
        }

        /// <summary>
        /// Sends a move action to the server
        /// </summary>
        public async Task SendMoveAsync(double x, double y, double z)
        {
            var moveData = new
            {
                x = x,
                y = y,
                z = z
            };

            await _client.SendGameActionAsync("move", moveData);
        }

        /// <summary>
        /// Sends a shoot action to the server
        /// </summary>
        public async Task SendShootAsync(double x, double y, double z, object velocity)
        {
            var shootData = new
            {
                x = x,
                y = y,
                z = z,
                velocity = velocity
            };

            await _client.SendGameActionAsync("shoot", shootData);
        }

        /// <summary>
        /// Sends a custom game action to the server
        /// </summary>
        public async Task SendActionAsync(string actionType, object actionData)
        {
            await _client.SendGameActionAsync(actionType, actionData);
        }
    }
}

