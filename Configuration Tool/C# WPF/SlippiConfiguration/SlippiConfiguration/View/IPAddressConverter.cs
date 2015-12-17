using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Data;

namespace Fizzi.Applications.SlippiConfiguration.View
{
    class IPAddressConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return convertIPAddress(value, targetType);
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return convertIPAddress(value, targetType);
        }

        private object convertIPAddress(object value, Type targetType)
        {
            try
            {
                if (targetType == typeof(string) && value is IPAddress)
                {
                    var ip = (IPAddress)value;
                    return ip.ToString();
                }
                else if (targetType == typeof(IPAddress) && value is string)
                {
                    var ipString = (string)value;
                    return IPAddress.Parse(ipString);
                }
                else return new ValidationResult(false, value);
            }
            catch (Exception)
            {
                return new ValidationResult(false, value);
            }
        }
    }
}
