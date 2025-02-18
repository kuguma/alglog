using UnityEngine;
using UnityEngine.TestTools;
using NUnit.Framework;
using System.Collections;
using System.IO;
using System.Threading.Tasks;
using System.Threading;

public class AlgLogTests
{
    private AlgLogBridge.Logger logger;

    [SetUp]
    public void Setup()
    {
        logger = new AlgLogBridge.Logger();
    }

    [TearDown]
    public void Cleanup()
    {
        if (logger != null)
        {
            logger.Flush();
            logger = null;
        }
    }

    [Test]
    public void BasicLoggingTest()
    {
        logger.Info("Basic logging test");
        logger.Error("Error message");
        logger.Alert("Alert message");
        logger.Critical("Critical message");
        logger.Warn("Warning message");
        logger.Debug("Debug message");
        logger.Trace("Trace message");
        logger.Flush();
        Assert.Pass("Basic logging completed");
    }

    [Test]
    public void StandardOutputTest()
    {
        // EBADFエラーの再現テスト
        for (int i = 0; i < 1000; i++)
        {
            logger.Debug($"Debug message #{i}");
        }
        logger.Flush();
        Assert.Pass("Standard output test completed");
    }

    [Test]
    public void MultiThreadedLoggingTest()
    {
        var tasks = new Task[10];
        for (int t = 0; t < tasks.Length; t++)
        {
            int threadId = t;
            tasks[t] = Task.Run(() =>
            {
                for (int i = 0; i < 100; i++)
                {
                    logger.Trace($"Thread {threadId}: Message {i}");
                    Thread.Sleep(1);
                }
            });
        }
        Task.WaitAll(tasks);
        logger.Flush();
        Assert.Pass("Multi-threaded logging test completed");
    }

    [Test]
    public void AsyncLoggingTest()
    {
        var asyncLogger = new AlgLogBridge.Logger(true);
        for (int i = 0; i < 10000; i++)
        {
            asyncLogger.Trace($"Async message #{i}");
        }
        asyncLogger.Flush();
        Assert.Pass("Async logging test completed");
    }
}
