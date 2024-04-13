using BepInEx;

namespace LethalRecordingMod
{
    [BepInPlugin("TheNerds", "LethalRecordingMod", "0.0.1")]
    public class Plugin : BaseUnityPlugin
    {
        private void Awake()
        {
            // Plugin startup logic
            Logger.LogInfo($"Plugin LethalRecordingMod hath become loaded!");
            Logger.LogInfo(">:(");
        }
    }
}
