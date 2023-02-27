using System;
using System.Drawing;
using System.IO;

namespace LogDataToPngImage
{
    class Program
    {
        static void Main(string[] args)
        {
            using (FileStream dest = new FileStream("image_data.bin", FileMode.Create, FileAccess.Write))
            {
                foreach (string line in File.ReadLines(@"log_data.txt"))
                {
                    if (!line.StartsWith(" 0x")) continue;

                    string hexValues = line.Replace(" 0x", "");
                    hexValues = hexValues.Replace(",", " ");
                    hexValues = hexValues.Trim();
                    string[] hexValuesSplit = hexValues.Split(' ');
                    foreach (string hex in hexValuesSplit)
                    {
                        byte value = Convert.ToByte(hex, 16);
                        dest.WriteByte(value);
                    }
                }
            }

            int frame = 1;
            const int BUFSIZE = 128 / 8 * 64;
            byte[] buf = new byte[BUFSIZE];
            using (FileStream src = new FileStream("image_data.bin", FileMode.Open, FileAccess.Read))
            {
                while (true)
                {
                    int readSize;
                    try
                    {
                        readSize = src.Read(buf, 0, BUFSIZE);
                    }
                    catch
                    {
                        continue;
                    }
                    if (readSize == 0)
                    {
                        break;
                    }

                    Bitmap bmp = new Bitmap(128, 64);

                    for (int y = 0; y < 64; ++y)
                    {
                        for (int x = 0; x < 16; ++x)
                        {
                            byte c = buf[x + y * 16];
                            for (int b = 0; b < 8; ++b)
                            {
                                Color col = ((0x80 >> b) & c) == 0 ? Color.Black: Color.White;
                                bmp.SetPixel((x*8)+b, y, col);
                            }
                        }
                    }

                    string fn = String.Format(@".\scale\{0:D5}.png", frame++);
                    bmp.Save(fn, System.Drawing.Imaging.ImageFormat.Png);

                    bmp.Dispose();
                }
            }
        }
    }
}

