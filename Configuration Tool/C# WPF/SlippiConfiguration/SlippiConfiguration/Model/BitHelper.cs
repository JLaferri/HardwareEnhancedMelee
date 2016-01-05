using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    static class BitHelper
    {
        public static byte ReadByte(byte[] msg, ref int index)
        {
            var value = msg[index];
            index++;

            return value;
        }

        public static UInt16 ReadUInt16(byte[] msg, ref int index)
        {
            var value = BitConverter.ToUInt16(msg, index);
            index += 2;

            return value;
        }

        public static UInt32 ReadUInt32(byte[] msg, ref int index)
        {
            var value = BitConverter.ToUInt32(msg, index);
            index += 4;

            return value;
        }

        public static float ReadFloat(byte[] msg, ref int index)
        {
            var value = BitConverter.ToSingle(msg, index);
            index += 4;

            return value;
        }
    }
}
