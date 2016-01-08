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
            var messageBuf = new byte[bytesToRead];

            while (messageSizeRead < bytesToRead)
            {
                var readCount = await stream.ReadAsync(messageBuf, messageSizeRead, bytesToRead - messageSizeRead);

                //Write down bytes read
                //System.Diagnostics.Trace.Write(string.Format("({0}) ", readCount));
                //for (int i = messageSizeRead; i < readCount + messageSizeRead; i++) System.Diagnostics.Trace.Write(string.Format("[{0:X2}]", messageBuf[i]));
                //System.Diagnostics.Trace.WriteLine("");

                messageSizeRead += readCount;
            }

            //System.Diagnostics.Trace.Write(string.Format("Complete: "));
            //for (int i = 0; i < bytesToRead; i++) System.Diagnostics.Trace.Write(string.Format("[{0:X2}]", messageBuf[i]));
            //System.Diagnostics.Trace.WriteLine("");

            return messageBuf;
        }
    }
}
