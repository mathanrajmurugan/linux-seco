#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/rational.h>
#include <linux/i2c.h>
#include <linux/of_platform.h>
#include <linux/platform_data/si5351.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/div64.h>
#include <linux/sysctl.h>



struct clk_consumer {
	struct device  *dev;
	struct clk     *clk;
};


static ssize_t clk_consumer_show_clk_freq (struct device *dev,
		struct device_attribute *attr, char *buf) {

	struct clk_consumer *consumer =
		(struct clk_consumer *)dev_get_drvdata(dev);

	if ( !consumer )
		return 0;


	return sprintf(buf, "%lu\n", clk_get_rate(consumer->clk));
}


static ssize_t clk_consumer_store_clk_freq (struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) {

	int rc;
	unsigned long rate;
	struct clk_consumer *consumer =
		(struct clk_consumer *)dev_get_drvdata(dev);

	if ( !consumer )
		return 0;

	rc = kstrtoul (buf, 0, &rate);
	if ( rc )
		return rc;

	clk_set_rate (consumer->clk, rate);

	return count;
}


static DEVICE_ATTR(frequency, 0644, clk_consumer_show_clk_freq, clk_consumer_store_clk_freq);

static struct attribute *clk_consumer_attrs[] = {
	&dev_attr_frequency.attr,
	NULL,
};

static struct attribute_group clk_consumer_attr_group = {
	.attrs = clk_consumer_attrs,
};


static int clk_consumer_probe (struct platform_device *pdev) {
	int sys_err, ret;

	struct clk_consumer *consumer;


	consumer = kzalloc (sizeof (struct clk_consumer), GFP_KERNEL);
	if ( !consumer ) {
		dev_err (&pdev->dev, "%s - failed to alloc resource\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata (pdev, consumer);

	consumer->clk = devm_clk_get(&pdev->dev, NULL);
	if ( IS_ERR (consumer->clk) ) {
		dev_err (&pdev->dev, "%s - input clock not found.\n", __func__);
		return PTR_ERR (consumer->clk);
	}
	ret = clk_prepare_enable (consumer->clk);
	if ( ret ) {
		dev_err (&pdev->dev, "%s - Unable to enable clock.\n", __func__);
		return ret;
	}

	consumer->dev = &pdev->dev;

	printk (KERN_INFO "%s - clock %s at rate %lu\n", __func__,
			__clk_get_name(consumer->clk), clk_get_rate(consumer->clk));

	sys_err = sysfs_create_group (&pdev->dev.kobj, &clk_consumer_attr_group);
	return 0;
}


static int clk_consumer_remove (struct platform_device *pdev) {
	return 0;
}


static const struct of_device_id clk_consumer_dt_ids[] = {
	{ .compatible = "seco,clk_consumer" },

	{ /*  sentinel */ }
};
MODULE_DEVICE_TABLE(of, clk_consumer_dt_ids);


static struct platform_driver clk_consumer_driver = {
	.driver = {
		.name = "clk-consumer",
		.of_match_table = of_match_ptr(clk_consumer_dt_ids),
	},
	.probe  = clk_consumer_probe,
	.remove = clk_consumer_remove,
};


static int __init clk_consumer_init(void) {
	return platform_driver_register (&clk_consumer_driver);
}
subsys_initcall(clk_consumer_init);


static void __exit clk_consumer__exit (void) {
	return platform_driver_unregister (&clk_consumer_driver);
}



MODULE_AUTHOR("Davide Cardillo <davide.cardillo@seco.com");
MODULE_DESCRIPTION("Generic clock consumer");
MODULE_LICENSE("GPL");
