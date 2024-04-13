using BepInEx;

namespace MyFirstPlugin
{
    [BepInPlugin("TheNerds", "LethalRecordingMod", "0.0.1")]
    public class Plugin : BaseUnityPlugin
    {
        private void Awake()
        {
            // Plugin startup logic
            Logger.LogInfo($"Plugin TheNerds.LethalRecordingMod is loaded!");
        }
    }
}
