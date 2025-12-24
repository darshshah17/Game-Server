using System;
using System.Threading.Tasks;

namespace GameServerSDK
{
    /// <summary>
    /// High-level API for chat operations
    /// </summary>
    public class ChatAPI
    {
        private readonly GameServerClient _client;

        public ChatAPI(GameServerClient client)
        {
            _client = client;
        }

        /// <summary>
        /// Sends a message to the global chat
        /// </summary>
        public async Task SendMessageAsync(string message)
        {
            await _client.SendChatMessageAsync(message, "global");
        }

        /// <summary>
        /// Sends a message to a specific channel
        /// </summary>
        public async Task SendMessageAsync(string message, string channel)
        {
            await _client.SendChatMessageAsync(message, channel);
        }
    }
}

