﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
// For device info
using Un4seen.Bass;

namespace KeppySynthConfigurator
{
    public partial class KeppySynthDefaultOutput : Form
    {
        public KeppySynthDefaultOutput()
        {
            InitializeComponent();
        }

        private void KeppySynthDefaultOutput_Load(object sender, EventArgs e)
        {
            BASS_DEVICEINFO info = new BASS_DEVICEINFO();
            DevicesList.Items.Add("Default Windows audio output");
            Bass.BASS_Init(-1, 44100, BASSInit.BASS_DEVICE_NOSPEAKER, IntPtr.Zero);
            int curdev = Bass.BASS_GetDevice();
            Bass.BASS_Free();
            Bass.BASS_GetDeviceInfo(curdev, info);
            if (info.ToString() == "")
            {
                DefOut.Text = String.Format("Default Windows output: No devices have been found", info.ToString());
            }
            else
            {
                DefOut.Text = String.Format("Default Windows output: {0}", info.ToString());
            }
            for (int n = 0; Bass.BASS_GetDeviceInfo(n, info); n++)
            {
                DevicesList.Items.Add(info.ToString());
            }
            DevicesList.SelectedIndex = (int)KeppySynthConfiguratorMain.SynthSettings.GetValue("defaultdev", 0);
        }

        private void DevicesList_SelectedIndexChanged(object sender, EventArgs e)
        {
            Functions.SetDefaultDevice(DevicesList.SelectedIndex);
        }

        private void Quit_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}
