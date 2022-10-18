using FrooxEngine;
using HarmonyLib;
using ManagedBass;
using NeosModLoader;
using System.IO;

namespace StartupSounds
{
    public class StartupSounds : NeosMod
    {
        public override string Name => "StartupSounds";
        public override string Author => "dfgHiatus";
        public override string Version => "1.0.0";
        public override string Link => "https://github.com/dfgHiatus/StartupSounds";

        public static ModConfiguration config;

        [AutoRegisterConfigKey]
        public static readonly ModConfigurationKey<string> soundfile = new ModConfigurationKey<string>("soundfile", "Sound File to be played", () => "WaterLily.ogg");

        public override void OnEngineInit()
        {
            config = GetConfiguration();

            var soundCanidate = Path.Combine("nml_mods", "startup_sounds", config.GetValue(soundfile));
            if (!File.Exists(soundCanidate))
            {
                Error("Sound file not found: " + soundCanidate);
                return;
            }

            if (!Bass.Init())
            {
                Error("Bass failed to initialize! " + Bass.LastError);
                Error("This is likely due to the fact that you are missing the bass.dll and/or ManagedBass.dll file in your base directory. Please ensure they are there!");
                return;
            }

            int stream = Bass.CreateStream(soundCanidate);
            Bass.ChannelPlay(stream, true);

            Engine.Current.OnReady += () =>
            {
                Bass.Stop();
                Bass.StreamFree(stream);
            };

            new Harmony("net.dfgHiatus.StartupSounds").PatchAll();
        }
    }
}