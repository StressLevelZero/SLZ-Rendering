using System;

namespace UnityEngine.Rendering.Universal
{
    [Serializable]
    [SupportedOnRenderPipeline(typeof(UniversalRenderPipelineAsset))]
    [Categorization.CategoryInfo(Name = "S: SLZ Runtime Resources", Order = 1000), HideInInspector]
    public class SlzRpRuntimeResources : IRenderPipelineResources
    {
        [SerializeField][HideInInspector] private int m_Version = 1;

        /// <summary>
        /// Current version of the resource container. Used only for upgrading a project.
        /// </summary>
        public int version => m_Version;

        bool IRenderPipelineGraphicsSettings.isAvailableInPlayerBuild => true;

        [SerializeField]
        [ResourcePath("Textures/FGD/FGD_GGX_Unorm16.png")]
        private Texture2D m_FgdGgxLut;

        /// <summary>
        /// Pre-baked blue noise textures.
        /// </summary>
        public Texture2D fdgGgxLut
        {
            get => m_FgdGgxLut;
            set => this.SetValueAndNotify(ref m_FgdGgxLut, value, nameof(m_FgdGgxLut));
        }
    }
}
