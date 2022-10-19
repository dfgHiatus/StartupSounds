using FrooxEngine;
using HarmonyLib;
using ManagedBass;
using NeosModLoader;
using System;
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
        public static readonly ModConfigurationKey<string?> soundDir = new ModConfigurationKey<string?>("soundDir", "Optional startup sounds directory. Leave empty to use the default \"startup_sounds\" folder", () => null);

        [AutoRegisterConfigKey]
        public static readonly ModConfigurationKey<string> soundfile = new ModConfigurationKey<string>("soundfile", "The sound file to be played", () => "WaterLily.ogg");

        [AutoRegisterConfigKey]
        public static readonly ModConfigurationKey<bool> random = new ModConfigurationKey<bool>("random", "Play a random song on startup", () => false);

        private static readonly string defaultStartupSoundsPath = Path.Combine("nml_mods", "startup_sounds"); 

        public override void OnEngineInit()
        {
            config = GetConfiguration();
            
            // Determine if the user's custom sound directory exists and is valid
            bool usingSoundDir = false;
            if (Directory.Exists(config.GetValue(soundDir)))
            {
                usingSoundDir = true;
            }
            else
            {
                Msg($"The provided custom sound directory was invalid. Using the default sound directory: {defaultStartupSoundsPath}");
            }

            // There are 4 states we need to check. In the event we cannot load any audio, these methods will throw a FileNotFoundException
            string soundCanidate;
            if (usingSoundDir)
            {
                // 1. The user has set a custom sound directory, is using random, and a sound file exists in the directory
                if (config.GetValue(random))
                {
                    soundCanidate = GetRandomAudioFile(config.GetValue(soundDir));
                }
                // 2. The user has set a custom sound directory, is not using random, and their sound file exists
                else
                {
                    soundCanidate = GetAudioFile(config.GetValue(soundDir), config.GetValue(soundfile));
                }
            }
            else
            {
                // 3. The user hasn't set a custom sound directory, is using random, and a sound file exists in the directory
                if (config.GetValue(random))
                {
                    soundCanidate = GetRandomAudioFile(defaultStartupSoundsPath);
                }
                // 4. The user hasn't set a custom sound directory, is not using random, and their sound file doesn't exist
                else
                {
                    soundCanidate = GetAudioFile(defaultStartupSoundsPath, config.GetValue(soundfile));
                }
            }

            if (!Bass.Init())
            {
                Error("Bass failed to initialize! " + Bass.LastError);
                Error("This is likely due to the fact that you are missing the bass.dll and/or ManagedBass.dll file in your base directory. Please ensure they are there!");
                return;
            }

            int stream = Bass.CreateStream(soundCanidate);
            Bass.ChannelPlay(stream, Restart: true);

            Engine.Current.OnReady += () =>
            {
                Bass.Stop();
                Bass.StreamFree(stream);
            };
            
            new Harmony("net.dfgHiatus.StartupSounds").PatchAll();
        }

        private static string GetRandomAudioFile(string dir)
        {           
            string[] files = Directory.GetFiles(dir, "\\.(?:wav|flac|ogg|mp3)$", SearchOption.AllDirectories);
            
            if (files.Length == 0) 
            {
                if (dir == defaultStartupSoundsPath) // If we get here, then we have searched the custom sound AND default directory and found nothing!
                {
                    throw new FileNotFoundException("No audio files were found in the default sound directory!");
                }

                Msg("No audio files were found in the custom sound directory. Trying to load from the default sound directory...");
                return GetRandomAudioFile(defaultStartupSoundsPath);
            }
            else
            {
                return files[new Random().Next(0, files.Length - 1)];
            }
        }

        private static string GetAudioFile(string path, string file)
        {
            var combined = Path.Combine(path, file);
            if (!File.Exists(combined))
            {
                throw new FileNotFoundException($"{file} was not found in {path}. Please check your sound directory and sound file name");
            }
            else
            {
                return combined;
            }
        }
    }
}