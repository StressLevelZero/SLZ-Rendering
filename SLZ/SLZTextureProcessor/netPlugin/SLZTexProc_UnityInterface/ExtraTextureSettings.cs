//using System;
using UnityEditor;
using UnityEngine;
using System.Xml.Serialization;
using System.IO;
using System;
using Object = UnityEngine.Object;

namespace SLZ.SLZTextureProcessor
{
    [Serializable]
    public class ExtraTextureSettings
    {
        public static int currentVersion = 1;

        public bool detailMap;
        public bool hemiOctNormals;
        public GeoRoughness geoRoughness;
        public float geoRoughnessPow;
        public bool dontReadSource;

        public int fileVersion = 0;
        public int settingsVersion = 0;

        public enum GeoRoughness : int
        {
            GenRoughness,
            NoRoughness,
            PreserveZ
        }

        public enum ReturnCase
        {
            Success = 0,
            OldXml = 1,
            Fail = 2,
        }

        public ExtraTextureSettings()
        {
            hemiOctNormals = true;
            geoRoughness = GeoRoughness.GenRoughness;
            geoRoughnessPow = 1.0f;
            dontReadSource = false;
            settingsVersion = currentVersion;
        }

        public static ExtraTextureSettings ReadFromUserData(string userData)
        {
            ExtraTextureSettings settings = new ExtraTextureSettings();
            if (userData != null && userData.Length > 1)
            {
                XmlSerializer serializer = new XmlSerializer(typeof(ExtraTextureSettings));
                TextReader reader = new StringReader(userData);
                try
                {
                    settings = JsonUtility.FromJson<ExtraTextureSettings>(userData);
                }
                catch
                {
                    try
                    {
                        settings = (ExtraTextureSettings)serializer.Deserialize(reader);
                    }
                    catch
                    {

                    }
                }
                reader.Close();
                reader.Dispose();
            }
            return settings;
        }

        public static ReturnCase TryReadFromUserData(string userData, out ExtraTextureSettings settings)
        {
            ReturnCase returnCode = ReturnCase.Success;
            if (userData != null && userData.Length > 1)
            {
                try
                {
                    settings = JsonUtility.FromJson<ExtraTextureSettings>(userData);
                }
                catch
                {
                    XmlSerializer serializer = new XmlSerializer(typeof(ExtraTextureSettings));
                    TextReader reader = new StringReader(userData);
                    returnCode = ReturnCase.OldXml;
                    try
                    {
                        settings = (ExtraTextureSettings)serializer.Deserialize(reader);
                    }
                    catch
                    {
                        settings = new ExtraTextureSettings();
                        returnCode = ReturnCase.Fail;
                    }
                    reader.Close();
                    reader.Dispose();
                }
            }
            else
            {
                settings = new ExtraTextureSettings();
                returnCode = ReturnCase.Fail;
            }
            return returnCode;
        }

        public string Serialize()
        {
            string settings = JsonUtility.ToJson(this,false);
            return settings;
        }
    }


    /// <summary>
    /// Janky UI that shows up in the header for the textureimporter inspector 
    /// </summary>
    [InitializeOnLoad]
    public class ExtraTexSettingsImporterUI
    {
        static int[] instanceIDs; 
        static ExtraTextureSettings[] texSettings;
        static ExtraTexSettingsImporterUI()
        {
            Editor.finishedDefaultHeaderGUI += CreateInspectorGUI;
            EditorApplication.playModeStateChanged += removeInspector;
        }

        static void removeInspector(PlayModeStateChange stateChange)
        {
            //if (stateChange == PlayModeStateChange.EnteredPlayMode)
            //{
            //Editor.finishedDefaultHeaderGUI -= CreateInspectorGUI;
            //}
        }

        public static void CreateInspectorGUI(Editor editor)
        {
            bool allTextureImporters = true;
            foreach (Object target in editor.targets) 
                allTextureImporters = allTextureImporters && (target.GetType() == typeof(TextureImporter));
            if (allTextureImporters && editor.targets.Length > 0)
            {
                //Debug.Log("Num editors: " + editor.targets.Length);
                //Debug.Log("Is targets[0] null: " + editor.targets[0] == null);
                bool selectionChanged = instanceIDs != null ? editor.targets.Length != instanceIDs.Length : true;
                //Debug.Log("No instanceIDs: " + (instanceIDs == null).ToString());
                bool hasNormalMap = false;
                int[] currentInstIDs = new int[editor.targets.Length];
                TextureImporter[] textureImporters = new TextureImporter[editor.targets.Length];
                for (int i = 0; i < editor.targets.Length; i++)
                {
                    currentInstIDs[i] = editor.targets[i].GetInstanceID();
                    selectionChanged = instanceIDs != null ? selectionChanged || (currentInstIDs[i] != instanceIDs[i]) : true;
                    textureImporters[i] = (TextureImporter)editor.targets[i];
                    
                    hasNormalMap = hasNormalMap || (textureImporters[i].textureType == TextureImporterType.NormalMap);
                }
                //Debug.Log("Successfully checked all importers");
                if (selectionChanged)
                {
                    XmlSerializer serializer = new XmlSerializer(typeof(ExtraTextureSettings));
                    ExtraTextureSettings[] newSettings = new ExtraTextureSettings[editor.targets.Length];
                    for (int i = 0; i < editor.targets.Length; i++)
                    {
                        //TextReader reader = new StringReader(textureImporters[i].userData);

                        //newSettings[i] = (ExtraTextureSettings)serializer.Deserialize(reader);
                        ExtraTextureSettings.ReturnCase c = ExtraTextureSettings.TryReadFromUserData(textureImporters[i].userData, out newSettings[i]);

                        if (c == ExtraTextureSettings.ReturnCase.Fail)
                        {
                            newSettings[i] = new ExtraTextureSettings();
                            string assetPath = AssetDatabase.GetAssetPath(editor.targets[i]);
                            string filename = Path.GetFileNameWithoutExtension(assetPath).ToLower();
                            if (filename.EndsWith("_detailmap"))
                            {
                                newSettings[i].detailMap = true;
                                newSettings[i].hemiOctNormals = false;
                                hasNormalMap = true;
                            }
                            //textureImporters[i].userData = newSettings[i].Serialize();
                        }
                        else if (c == ExtraTextureSettings.ReturnCase.OldXml)
                        {
                            //textureImporters[i].userData = newSettings[i].Serialize();
                        }
                    }
                    texSettings = newSettings;
                    instanceIDs = currentInstIDs;
                    //Debug.Log("Successfully deserialized data");
                }

                for (int i = 0; i < texSettings.Length; i++)
                {
                    hasNormalMap = hasNormalMap || texSettings[i].detailMap;
                }
                //TextureImporter importer = (TextureImporter)editor.target;
                
                EditorGUILayout.LabelField("General Settings");
                SerializedProperty mipBias = editor.serializedObject.FindProperty("m_TextureSettings.m_MipBias");
                //Debug.Log(mipBias == null);
                mipBias.floatValue = EditorGUILayout.Slider("Mipmap bias", mipBias.floatValue, -12, 12);
                editor.serializedObject.ApplyModifiedProperties();
                //editor.serializedObject
                /*
                if (EditorGUI.EndChangeCheck())
                {
                    EditorUtility.SetDirty(importer);
                    AssetDatabase.SaveAssetIfDirty(importer);
                }
                */
                
                EditorGUI.BeginChangeCheck();
                GUIContent detailCont = new GUIContent("Detail Map", "Texture containing\n R: Desaturated Albedo Mask,\n G: normal Y,\n B: 0-2 Smoothness Multiplier,\n A: normal X");
                texSettings[0].detailMap = EditorGUILayout.Toggle(detailCont, texSettings[0].detailMap);

                if (EditorGUI.EndChangeCheck())
                {
                    //XmlSerializer serializer = new XmlSerializer(typeof(ExtraTextureSettings));
                    for (int i = 0; i < editor.targets.Length; i++)
                    {
                        texSettings[i].detailMap = texSettings[0].detailMap;
                        //StringWriter stringWriter = new StringWriter();
                        string usrData = texSettings[i].Serialize();
                        textureImporters[i].userData = usrData;
                    }
                }


                Rect ControlRect = EditorGUILayout.GetControlRect();

                if (hasNormalMap)
                {
                    EditorGUILayout.LabelField("Normal Map Settings");



                    GUIContent roughCont = new GUIContent("Geometric Roughness", "In the blue channel of the normal map, do we store the smoothness of each mip pixel defined by the distribution of normals from the source image averaged to make that pixel" +
                "or store a flat value of 1 so that shaders expecting smoothness information are unaffected by the normal map.");
                //"if hemi-oct encoding is disabled, setting it to \"No Geometric Roughness\" preserves the original blue channel");

                    GUIContent[] rghOptions = new GUIContent[] {
                        new GUIContent("Store Geometric Roughness", "For each pixel at each mip level, calculate the standard deviations of the x and y components of the pixels from" +
                        " the source resolution image that contributed to the pixel and average them. This approximately represents the isotropic roughness inherit to the geometry of the normal map, and is stored in the blue channel"),
                        new GUIContent("No geometric roughness", "Fills the blue channel with a value of 0 (perfectly smooth)") 
                    };
                        //new GUIContent("Preserve original blue channel", "Keep the image's original blue channel. For shaders that expect other information in the blue channel, like the XYZ normal format used on Android")};

                    EditorGUI.BeginChangeCheck();
                    texSettings[0].geoRoughness = (ExtraTextureSettings.GeoRoughness)EditorGUILayout.Popup(roughCont, (int)texSettings[0].geoRoughness, rghOptions, EditorStyles.popup);
                    if (EditorGUI.EndChangeCheck())
                    {
                        
                        for (int i = 0; i < editor.targets.Length; i++)
                        {
                            texSettings[i].geoRoughness = texSettings[0].geoRoughness;
                            string usrData = texSettings[i].Serialize();
                            textureImporters[i].userData = usrData;
                        }
                    }

                    GUIContent powCont = new GUIContent("Geometric Roughness Strength", "Fudge factor" +
                                " for the calculated geometric roughness values. The higher the power, the rougher it gets." +
                                " Works by raising the geometric smoothness to the specified power.");

                    EditorGUI.BeginChangeCheck();
                    texSettings[0].geoRoughnessPow = EditorGUILayout.FloatField(powCont, texSettings[0].geoRoughnessPow);
                    if (EditorGUI.EndChangeCheck())
                    {
                       
                        for (int i = 0; i < editor.targets.Length; i++)
                        {
                            texSettings[i].geoRoughnessPow = texSettings[0].geoRoughnessPow;
                            string usrData = texSettings[i].Serialize();
                            textureImporters[i].userData = usrData;
                        }
                    }

                    GUIContent hemiCont = new GUIContent("Hemi-Octahedral Normal Encoding", 
                        "From the normal vectors stored in the map, calculate x,y octahedral coordinates and store those instead." +
                        " Octahedral coordinates with >1 lengths are valid, adding roughly 20% more precision over storing the raw" +
                        " normal vector. Note that to get this precision boost, the original image must have been created in a high precision format " +
                        " having 16 bits per color channel or more.\n\n WARNING: shaders must support hemi-oct normals to use this, and shaders that support" +
                        " hemi-oct normals can only use hemi-oct normal maps. Unity's default unpacking functions have been modified" +
                        " to use hemi-octahedral normals by default. Define 'USE_STANDARD_NORMALMAPS' in shaders to revert to the" +
                        " original behavior");

                  
                    EditorGUI.BeginChangeCheck();
                    texSettings[0].hemiOctNormals = EditorGUILayout.Toggle(hemiCont, texSettings[0].hemiOctNormals);
                    if (EditorGUI.EndChangeCheck())
                    {
                        XmlSerializer serializer = new XmlSerializer(typeof(ExtraTextureSettings));
                        for (int i = 0; i < editor.targets.Length; i++)
                        {
                            texSettings[i].hemiOctNormals = texSettings[0].hemiOctNormals;
                            string usrData = texSettings[i].Serialize();
                            textureImporters[i].userData = usrData;
                        }
                    }

                    EditorGUI.BeginChangeCheck();
                    GUIContent sourceCont = new GUIContent("Use preprocessed texture", "During the import process, let unity handle the reading of the image instead of the internal libraries." +
                        " This means that the importer will get a low resolution/low bit depth version of the image to work with, but it may be necessary for certain files that the library can't" +
                        " read correctly but unity can (for example pngs with photoshop colorspaces defined in the EXIF header)");
                    texSettings[0].dontReadSource = EditorGUILayout.Toggle(sourceCont, texSettings[0].dontReadSource);

                    if (EditorGUI.EndChangeCheck())
                    {
                        XmlSerializer serializer = new XmlSerializer(typeof(ExtraTextureSettings));
                        for (int i = 0; i < editor.targets.Length; i++)
                        {
                            texSettings[i].dontReadSource = texSettings[0].dontReadSource;
                            string usrData = texSettings[i].Serialize();
                            textureImporters[i].userData = usrData;
                        }
                    }
                }
            }
        }
    }
}
