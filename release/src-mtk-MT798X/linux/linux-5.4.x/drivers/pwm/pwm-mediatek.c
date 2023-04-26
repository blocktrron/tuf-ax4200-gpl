// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek Pulse Width Modulator driver
 *
 * Copyright (C) 2015 John Crispin <blogic@openwrt.org>
 * Copyright (C) 2017 Zhi Mao <zhi.mao@mediatek.com>
 *
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

/* PWM registers and bits definitions */
#define PWMCON			0x00
#define PWMHDUR			0x04
#define PWMLDUR			0x08
#define PWMGDUR			0x0c
#define PWMWAVENUM		0x28
#define PWMDWIDTH		0x2c
#define PWM45DWIDTH_FIXUP	0x30
#define PWMTHRES		0x30
#define PWM45THRES_FIXUP	0x34

#define PWM_CLK_DIV_MAX		7
#define REG_V1			1
#define REG_V2			2

struct pwm_mediatek_of_data {
	unsigned int num_pwms;
	bool pwm45_fixup;
	int reg_ver;
};

/**
 * struct pwm_mediatek_chip - struct representing PWM chip
 * @chip: linux PWM chip representation
 * @regs: base address of PWM chip
 * @clk_top: the top clock generator
 * @clk_main: the clock used by PWM core
 * @clk_pwms: the clock used by each PWM channel
 * @clk_freq: the fix clock frequency of legacy MIPS SoC
 */
struct pwm_mediatek_chip {
	struct pwm_chip chip;
	void __iomem *regs;
	struct clk *clk_top;
	struct clk *clk_main;
	struct clk **clk_pwms;
	const struct pwm_mediatek_of_data *soc;
};

static const unsigned int mtk_pwm_reg_offset_v1[] = {
	0x0010, 0x0050, 0x0090, 0x00d0, 0x0110, 0x0150, 0x0190, 0x0220
};

static const unsigned int mtk_pwm_reg_offset_v2[] = {
	0x0080, 0x00c0, 0x0100, 0x0140, 0x0180, 0x1c0, 0x200, 0x0240
};

inline struct pwm_mediatek_chip *
to_pwm_mediatek_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct pwm_mediatek_chip, chip);
}

int pwm_mediatek_clk_enable(struct pwm_chip *chip,
				   struct pwm_device *pwm)
{
	struct pwm_mediatek_chip *pc = to_pwm_mediatek_chip(chip);
	int ret;

	ret = clk_prepare_enable(pc->clk_top);
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(pc->clk_main);
	if (ret < 0)
		goto disable_clk_top;

	ret = clk_prepare_enable(pc->clk_pwms[pwm->hwpwm]);
	if (ret < 0)
		goto disable_clk_main;

	return 0;

disable_clk_main:
	clk_disable_unprepare(pc->clk_main);
disable_clk_top:
	clk_disable_unprepare(pc->clk_top);

	return ret;
}

void pwm_mediatek_clk_disable(struct pwm_chip *chip,
				     struct pwm_device *pwm)
{
	struct pwm_mediatek_chip *pc = to_pwm_mediatek_chip(chip);

	clk_disable_unprepare(pc->clk_pwms[pwm->hwpwm]);
	clk_disable_unprepare(pc->clk_main);
	clk_disable_unprepare(pc->clk_top);
}

inline u32 pwm_mediatek_readl(struct pwm_mediatek_chip *chip,
				     unsigned int num, unsigned int offset)
{
	u32 pwm_offset;

	switch (chip->soc->reg_ver) {
	case REG_V2:
		pwm_offset = mtk_pwm_reg_offset_v2[num];
		break;

	case REG_V1:
	default:
		pwm_offset = mtk_pwm_reg_offset_v1[num];
	}

	return readl(chip->regs + pwm_offset + offset);
}

inline void pwm_mediatek_writel(struct pwm_mediatek_chip *chip,
				       unsigned int num, unsigned int offset,
				       u32 value)
{
	u32 pwm_offset;

	switch (chip->soc->reg_ver) {
	case REG_V2:
		pwm_offset = mtk_pwm_reg_offset_v2[num];
		break;

	case REG_V1:
	default:
		pwm_offset = mtk_pwm_reg_offset_v1[num];
	}

	writel(value, chip->regs + pwm_offset + offset);
}

static int pwm_mediatek_config(struct pwm_chip *chip, struct pwm_device *pwm,
			       int duty_ns, int period_ns)
{
	struct pwm_mediatek_chip *pc = to_pwm_mediatek_chip(chip);
	u32 clkdiv = 0, cnt_period, cnt_duty, reg_width = PWMDWIDTH,
	    reg_thres = PWMTHRES;
	u64 resolution;
	int ret;

	ret = pwm_mediatek_clk_enable(chip, pwm);

	if (ret < 0)
		return ret;

	/* Using resolution in picosecond gets accuracy higher */
	resolution = (u64)NSEC_PER_SEC * 1000;
	do_div(resolution, clk_get_rate(pc->clk_pwms[pwm->hwpwm]));

	cnt_period = DIV_ROUND_CLOSEST_ULL((u64)period_ns * 1000, resolution);
	while (cnt_period > 8191) {
		resolution *= 2;
		clkdiv++;
		cnt_period = DIV_ROUND_CLOSEST_ULL((u64)period_ns * 1000,
						   resolution);
	}

	if (clkdiv > PWM_CLK_DIV_MAX) {
		pwm_mediatek_clk_disable(chip, pwm);
		dev_err(chip->dev, "period %d not supported\n", period_ns);
		return -EINVAL;
	}

	if (pc->soc->pwm45_fixup && pwm->hwpwm > 2) {
		/*
		 * PWM[4,5] has distinct offset for PWMDWIDTH and PWMTHRES
		 * from the other PWMs on MT7623.
		 */
		reg_width = PWM45DWIDTH_FIXUP;
		reg_thres = PWM45THRES_FIXUP;
	}

	cnt_duty = DIV_ROUND_CLOSEST_ULL((u64)duty_ns * 1000, resolution);
	pwm_mediatek_writel(pc, pwm->hwpwm, PWMCON, BIT(15) | clkdiv);
	pwm_mediatek_writel(pc, pwm->hwpwm, reg_width, cnt_period);
	pwm_mediatek_writel(pc, pwm->hwpwm, reg_thres, cnt_duty);

	pwm_mediatek_clk_disable(chip, pwm);

	return 0;
}

static int pwm_mediatek_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct pwm_mediatek_chip *pc = to_pwm_mediatek_chip(chip);
	u32 value;
	int ret;

	ret = pwm_mediatek_clk_enable(chip, pwm);
	if (ret < 0)
		return ret;

	value = readl(pc->regs);
	value |= BIT(pwm->hwpwm);
	writel(value, pc->regs);

	return 0;
}

static void pwm_mediatek_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct pwm_mediatek_chip *pc = to_pwm_mediatek_chip(chip);
	u32 value;

	value = readl(pc->regs);
	value &= ~BIT(pwm->hwpwm);
	writel(value, pc->regs);

	pwm_mediatek_clk_disable(chip, pwm);
}

#if 1 /* PWM memory mode */
int pwm_mtk_mm_pwmbuf(struct pwm_chip *chip, struct pwm_device *pwm, const char *type);
int pwm_mtk_mm_config(struct pwm_chip *chip, struct pwm_device *pwm, const char *config);
int pwm_mtk_mm_fint(struct pwm_chip *chip, struct pwm_device *pwm, const char *fint);
irqreturn_t pwm_mtk_interrupt(int irq, void *data);

extern struct workqueue_struct *pwm_mtk_wq;
extern struct work_struct work_pwm_mtk[2];
void pwm_mtk_wq_func0(struct work_struct *work);
void pwm_mtk_wq_func1(struct work_struct *work);
#endif

static const struct pwm_ops pwm_mediatek_ops = {
	.config = pwm_mediatek_config,
	.enable = pwm_mediatek_enable,
	.disable = pwm_mediatek_disable,
#if 1 /* PWM memory mode */
	.mm_pwmbuf = pwm_mtk_mm_pwmbuf,
	.mm_config = pwm_mtk_mm_config,
	.mm_fint = pwm_mtk_mm_fint,
#endif
	.owner = THIS_MODULE,
};

static int pwm_mediatek_probe(struct platform_device *pdev)
{
	struct pwm_mediatek_chip *pc;
	struct resource *res;
	unsigned int i;
	int ret;
#if 1 /* PWM memory mode */
	int irq;
#endif

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	pc->soc = of_device_get_match_data(&pdev->dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pc->regs))
		return PTR_ERR(pc->regs);

	pc->clk_pwms = devm_kcalloc(&pdev->dev, pc->soc->num_pwms,
				    sizeof(*pc->clk_pwms), GFP_KERNEL);
	if (!pc->clk_pwms)
		return -ENOMEM;

	pc->clk_top = devm_clk_get(&pdev->dev, "top");
	if (IS_ERR(pc->clk_top)) {
		dev_err(&pdev->dev, "clock: top fail: %ld\n",
			PTR_ERR(pc->clk_top));
		return PTR_ERR(pc->clk_top);
	}

	pc->clk_main = devm_clk_get(&pdev->dev, "main");
	if (IS_ERR(pc->clk_main)) {
		dev_err(&pdev->dev, "clock: main fail: %ld\n",
			PTR_ERR(pc->clk_main));
		return PTR_ERR(pc->clk_main);
	}

	for (i = 0; i < pc->soc->num_pwms; i++) {
		char name[8];

		snprintf(name, sizeof(name), "pwm%d", i + 1);

		pc->clk_pwms[i] = devm_clk_get(&pdev->dev, name);
		if (IS_ERR(pc->clk_pwms[i])) {
			dev_err(&pdev->dev, "clock: %s fail: %ld\n",
				name, PTR_ERR(pc->clk_pwms[i]));
			return PTR_ERR(pc->clk_pwms[i]);
		}
	}

	platform_set_drvdata(pdev, pc);

	pc->chip.dev = &pdev->dev;
	pc->chip.ops = &pwm_mediatek_ops;
	pc->chip.base = -1;
	pc->chip.npwm = pc->soc->num_pwms;

	ret = pwmchip_add(&pc->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

#if 1 /* PWM memory mode */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(&pdev->dev, irq, pwm_mtk_interrupt, 0,
			       pdev->name, pc);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request IRQ\n");
		return ret;
	}

	pwm_mtk_wq = alloc_workqueue("pwm_mtk_wq", WQ_HIGHPRI, 0);
	if (!pwm_mtk_wq)
		return -ENOMEM;
	/* pwm0 */
	INIT_WORK(&work_pwm_mtk[0], pwm_mtk_wq_func0);
	/* pwm1 */
	INIT_WORK(&work_pwm_mtk[1], pwm_mtk_wq_func1);
#endif

	return 0;
}

static int pwm_mediatek_remove(struct platform_device *pdev)
{
	struct pwm_mediatek_chip *pc = platform_get_drvdata(pdev);

	return pwmchip_remove(&pc->chip);
}

static const struct pwm_mediatek_of_data mt2712_pwm_data = {
	.num_pwms = 8,
	.pwm45_fixup = false,
	.reg_ver = REG_V1,
};

static const struct pwm_mediatek_of_data mt7622_pwm_data = {
	.num_pwms = 6,
	.pwm45_fixup = false,
	.reg_ver = REG_V1,
};

static const struct pwm_mediatek_of_data mt7623_pwm_data = {
	.num_pwms = 5,
	.pwm45_fixup = true,
	.reg_ver = REG_V1,
};

static const struct pwm_mediatek_of_data mt7628_pwm_data = {
	.num_pwms = 4,
	.pwm45_fixup = true,
	.reg_ver = REG_V1,
};

static const struct pwm_mediatek_of_data mt7629_pwm_data = {
	.num_pwms = 1,
	.pwm45_fixup = false,
	.reg_ver = REG_V1,
};

static const struct pwm_mediatek_of_data mt7981_pwm_data = {
	.num_pwms = 3,
	.pwm45_fixup = false,
	.reg_ver = REG_V2,
};

static const struct pwm_mediatek_of_data mt7986_pwm_data = {
	.num_pwms = 2,
	.pwm45_fixup = false,
	.reg_ver = REG_V1,
};

static const struct pwm_mediatek_of_data mt8516_pwm_data = {
	.num_pwms = 5,
	.pwm45_fixup = false,
	.reg_ver = REG_V1,
};

static const struct of_device_id pwm_mediatek_of_match[] = {
	{ .compatible = "mediatek,mt2712-pwm", .data = &mt2712_pwm_data },
	{ .compatible = "mediatek,mt7622-pwm", .data = &mt7622_pwm_data },
	{ .compatible = "mediatek,mt7623-pwm", .data = &mt7623_pwm_data },
	{ .compatible = "mediatek,mt7628-pwm", .data = &mt7628_pwm_data },
	{ .compatible = "mediatek,mt7629-pwm", .data = &mt7629_pwm_data },
	{ .compatible = "mediatek,mt7981-pwm", .data = &mt7981_pwm_data },
	{ .compatible = "mediatek,mt7986-pwm", .data = &mt7986_pwm_data },
	{ .compatible = "mediatek,mt8516-pwm", .data = &mt8516_pwm_data },
	{ },
};
MODULE_DEVICE_TABLE(of, pwm_mediatek_of_match);

static struct platform_driver pwm_mediatek_driver = {
	.driver = {
		.name = "pwm-mediatek",
		.of_match_table = pwm_mediatek_of_match,
	},
	.probe = pwm_mediatek_probe,
	.remove = pwm_mediatek_remove,
};
module_platform_driver(pwm_mediatek_driver);

MODULE_AUTHOR("John Crispin <blogic@openwrt.org>");
MODULE_LICENSE("GPL v2");
