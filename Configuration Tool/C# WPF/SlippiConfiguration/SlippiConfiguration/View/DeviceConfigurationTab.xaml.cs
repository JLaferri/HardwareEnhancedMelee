using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Fizzi.Applications.SlippiConfiguration.View
{
    /// <summary>
    /// Interaction logic for DeviceConfigurationTab.xaml
    /// </summary>
    public partial class DeviceConfigurationTab : UserControl
    {
        public DeviceConfigurationTab()
        {
            InitializeComponent();
        }

        private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            (sender as TextBox).GetBindingExpression(TextBox.TextProperty).ValidateWithoutUpdate();
        }
    }
}
