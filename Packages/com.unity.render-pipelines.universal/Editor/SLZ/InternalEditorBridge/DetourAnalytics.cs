using System.Reflection;
using UnityEngine;
using UnityEditor;
using System;
using UnityEngine.Analytics;
using System.Runtime.CompilerServices;

namespace SLZ.SLZEditorTools
{
    
    internal static class DetourAnalytics
    {
        [InitializeOnLoadMethod]
        static void Nuke()
        {
            System.Type EA = typeof(UnityEditor.EditorAnalytics);
            MethodInfo fakeVoid = ((Action)FakeVoid).Method;
            MethodInfo fakeResult = ((Func<AnalyticsResult>)FakeWithResult).Method;
            MethodInfo fakeBoolResult = AsMethodInfoF(FakeBoolResult);

            Detour.TryDetourFromTo(AsMethodInfoA<string>(EditorAnalytics.SendAnalyticsToEditor), fakeVoid);
            Detour.TryDetourFromTo(AsMethodInfoF(EditorAnalytics.RegisterEventEditorGameService), fakeBoolResult);
            Detour.TryDetourFromTo(((Func<Analytic, Assembly, AnalyticsResult>)EditorAnalytics.TryRegisterAnalytic).Method, fakeResult);
            Detour.TryDetourFromTo(((Func<Analytic, AnalyticsResult>)EditorAnalytics.TrySendAnalytic).Method, fakeResult);
            Detour.TryDetourFromTo(((Func<IAnalytic, AnalyticsResult>)EditorAnalytics.SendAnalytic).Method, fakeResult);
            Detour.TryDetourFromTo(((Func<Analytic, Assembly, AnalyticsResult>)EditorAnalytics.SendAnalytic).Method, fakeResult);

            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventRefreshAccess), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventNewLink), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventScriptableBuildPipelineInfo), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventBuildFrameworkList), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventServiceInfo), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventShowService), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventCloseServiceWindow), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventEditorGameService), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendImportServicePackageEvent), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendOpenPackManFromServiceSettings), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendOpenDashboardForService), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendValidatePublicKeyEvent), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendLaunchCloudBuildEvent), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendClearAnalyticsDataEvent), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendProjectServiceBindingEvent), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendCoppaComplianceEvent), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventTimelineInfo), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventBuildTargetDevice), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventSceneViewInfo), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventBuildPackageList), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendEventBuildTargetPermissions), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendCollabUserAction), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendCollabOperation), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendAssetPostprocessorsUsage), fakeResult);
            Detour.TryDetourFromTo(AsMethodInfoAO(EditorAnalytics.SendAssetDownloadEvent), fakeResult);
        }

        static UnityEngine.Analytics.AnalyticsResult FakeWithResult()
        {
            return UnityEngine.Analytics.AnalyticsResult.AnalyticsDisabled;
        }

        static bool FakeBoolResult()
        {
            return true;
        }

        static void FakeVoid()
        {
            return;
        }

        /*
        static void DetourMethod(Type type, string methodName, MethodInfo replacement)
        {
            MethodInfo mi = type.GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
            if (mi == null)
            {
                Debug.LogError($"Analytics Remover: Could not find matching method \"{methodName}\" in type {type.FullName}");
            }
            Detour.TryDetourFromTo(mi, replacement);
        }
        */
        static void DetourMethod(Type type, string methodName, MethodInfo replacement)
        {
            MethodInfo mi = type.GetMethod(methodName, BindingFlags.Static | BindingFlags.NonPublic);
            if (mi == null)
            {
                Debug.LogError($"Analytics Remover: Could not find matching method \"{methodName}\" in type {type.FullName}");
            }
            Detour.TryDetourFromTo(mi, replacement);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        static MethodInfo AsMethodInfo(Action action)
        {
            return action.Method;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        static MethodInfo AsMethodInfoA<T>(Action<T> action)
        {
            return action.Method;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        static MethodInfo AsMethodInfoF<T>(Func<T> action)
        {
            return action.Method;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        static MethodInfo AsMethodInfoAO(Func<object, AnalyticsResult> action)
        {
            return action.Method;
        }
    }
}