export class RuleHitCount {
    hash: string = '';
    hits: number = 0;

    constructor(hash, hits) {
      this.hash = hash;
      this.hits = hits;
    }
  }

export class TopRulesUtil {

    public static pieColors: string[] = [ 'rgba(90, 147, 246, 0.39)', 'rgba(236, 93, 87, 0.39)',
                                    'rgba(219, 0, 0, 0.39)', 'rgba(126, 211, 33, 0.39)',
                                    'rgba(129, 95, 219, 0.39)', 'rgba(192, 82, 168, 0.39)',
                                    'rgba(33, 211, 125, 0.39)', 'rgba(255, 132, 13, 0.39)',
                                    'rgba(255, 49, 210, 0.39)', 'rgba(0, 24, 220, 0.39)',
                                    'rgba(103, 103, 99, 0.38)'];

    public static reducePercentagePrecision(number: number) {
      if (number < 0.01) {
        return '<0.01';
      }
      const percent = number.toFixed(2);
      return percent;
    }

    public static getHitsText(hits: number) {
      const suffixList = ['', 'k', 'M', 'G'];
      // billion or giga?

      let tier = Math.floor((hits.toString().length - 1) / 3);
      tier = Math.min(tier, 3);

      const suffix = suffixList[tier];

      const short = Math.floor(hits / (10 ** (3 * tier)));
      return short + suffix;
    }
}
