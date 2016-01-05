using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.Model
{
    enum TcpMessageType
    {
        GameStart = 0x37,
        GameUpdate = 0x38,
        GameEnd = 0x39,
    }
}
