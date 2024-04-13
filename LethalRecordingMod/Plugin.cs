using BepInEx;
using BepInEx.Logging;
using HarmonyLib;

namespace LethalRecordingMod
{

   
    [BepInPlugin("TheNerds.LethalRecordingMod", "LethalRecordingMod", "0.0.1")]
    public class Plugin : BaseUnityPlugin
    {
        private readonly Harmony harmony = new Harmony("TheNerds.LethalRecordingMod");
        public static Plugin Instance;
        public ManualLogSource mls;
        private void Awake()
        {
            if (Instance == null) {
                Instance = this;
            } 
            // Plugin startup logic
            Logger.LogInfo($"Plugin LethalRecordingMod hath become loaded!");
            mls.LogInfo("We're up and running!");
        }
    }
}
