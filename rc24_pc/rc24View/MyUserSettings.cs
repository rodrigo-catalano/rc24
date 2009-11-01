/*
Copyright 2009 © Graham Bloice

	This file is part of rc24.

    rc24 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    rc24 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with rc24.  If not, see <http://www.gnu.org/licenses/>.


*/

using System;
using System.Configuration;

namespace Serial
{
    /// <summary>
    /// MyUserSettings provides the persistence of user settings.
    /// </summary>
    public class MyUserSettings : ApplicationSettingsBase
    {
        [UserScopedSetting()]
        [DefaultSettingValueAttribute(@"C:\Jennic\cygwin\jennic\SDK\Application\rc24\JN5139_Build\Release\rc24tx.bin")]
        public string txbinpath
        {
            get { return ((string)this["TxBinPath"]); }
            set { this["TxBinPath"] = (string)value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute(@"C:\Jennic\cygwin\jennic\SDK\Application\rc24\JN5139_Build\Release\rc24rx.bin")]
        public string rxbinpath
        {
            get { return ((string)this["RxBinPath"]); }
            set { this["RxBinPath"] = (string)value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute(@"C:\Jennic\Application\rc24\tx24\build\tx24_JN5148.bin")]
        public string tx5148binpath
        {
            get { return ((string)this["Tx5148BinPath"]); }
            set { this["Tx5148BinPath"] = (string)value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute(@"C:\Jennic\Application\rc24\rx24\build\rx24_JN5148.bin")]
        public string rx5148binpath
        {
            get { return ((string)this["Rx5148BinPath"]); }
            set { this["Rx5148BinPath"] = (string)value; }
        }

        [UserScopedSetting()]
        [DefaultSettingValueAttribute("COM7")]
        public string comPort
        {
            get { return ((string)this["ComPort"]); }
            set { this["ComPort"] = (string)value; }
        }
    }
}
