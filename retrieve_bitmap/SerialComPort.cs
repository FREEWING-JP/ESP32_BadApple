using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.IO;
using System.Threading;

namespace SerialComPort
{
    class Program
    {
        static void Main(string[] args)
        {
            // SerialPort.GetPortNames Method
            // https://learn.microsoft.com/en-us/dotnet/api/system.io.ports.serialport.getportnames?view=netframework-4.8
            // Get a list of serial port names.
            string[] ports = SerialPort.GetPortNames();

            Console.WriteLine("The following serial ports were found:");

            // Display each port name to the console.
            foreach (string port in ports)
            {
                Console.WriteLine(port);
            }

            Console.ReadLine();

            // Logging
            SerialPort sp = new SerialPort();
            sp.PortName = "COM7";
            sp.BaudRate = 921600;
            sp.Parity = Parity.None;
            sp.DataBits = 8;
            sp.StopBits = StopBits.One;
            sp.Handshake = Handshake.None;

            try
            {
                sp.Open();

                // RTS = Reset
                sp.RtsEnable = true;
                Thread.Sleep(1000);
                sp.RtsEnable = false;

                // sp.DtrEnable = true;
                // Thread.Sleep(1000);
                // sp.DtrEnable = false;

                // set read timeout
                sp.ReadTimeout = 5000;
            }
            catch (System.Exception ex)
            {
                Console.WriteLine(ex.Message);
            }


            while (true)
            {
                File.AppendAllText(@"log_data.txt", sp.ReadLine());
            }
        }
    }
}

