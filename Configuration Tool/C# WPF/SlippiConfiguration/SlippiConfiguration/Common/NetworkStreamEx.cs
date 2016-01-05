using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Common
{
    static class NetworkStreamEx
    {
        public async static Task<byte[]> ReadExactBytesAsync(this NetworkStream stream, int bytesToRead)
        {
            var messageSizeRead = 0;
            var messageSizeBuf = new byte[bytesToRead];

            while (messageSizeRead < bytesToRead)
            {
                var readCount = await stream.ReadAsync(messageSizeBuf, 0, bytesToRead);
                messageSizeRead += readCount;
            }

            return messageSizeBuf;
        }
    }
}
