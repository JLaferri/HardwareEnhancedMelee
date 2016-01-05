using Fizzi.Applications.SlippiConfiguration.Properties;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net.Sockets;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Fizzi.Applications.SlippiConfiguration.ViewModel
{
    class ServerViewModel
    {
        private TcpListener listener = null;

        public ObservableCollection<SlippiConnection> Connections { get; private set; }

        public ServerViewModel()
        {
            Connections = new ObservableCollection<SlippiConnection>();
        }

        public void StartServer()
        {
            //Don't start server unless it is currently not started
            if (listener == null)
            {
                //Indicate that the server is currently enabled
                Settings.Default.ServerEnabled = true;

                //Start listener on the configured port
                listener = TcpListener.Create(Settings.Default.ServerPort);
                listener.Start();

                //Listen for Tcp connections until server is disabled
                Observable.Start(() =>
                {
                    while(Settings.Default.ServerEnabled)
                    {
                        //Accept a client connection - blocking call
                        var client = listener.AcceptTcpClient();
                        
                        //Create a slippi connection object
                        var connection = new SlippiConnection(client);

                        //Add slippi connection to collection, prep it to be removed if an error is encountered
                        Connections.Add(connection);
                        connection.TerminateAction = () => Connections.Remove(connection);
                    }
                }, System.Reactive.Concurrency.TaskPoolScheduler.Default);
            }
        }

        public void StopServer()
        {
            //Don't attempt to stop server if it isn't already started
            if (listener != null)
            {
                //Mark server as disabled and stop server if it exists
                Settings.Default.ServerEnabled = false;

                //Stop server
                listener.Stop();
                listener = null;

                //Clear all connections
                Connections.Clear();
            }
        }
    }
}
